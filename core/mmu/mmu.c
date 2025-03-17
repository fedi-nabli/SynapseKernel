/*
 * mmu.c - This files implements kernel MMU functionalities
 * 
 * The functions are meant to control the MMU and
 * be architecture-independant
 * 
 * Author: Fedi Nabli
 * Date: 13 Mar 2025
 * Last Modified: 17 Mar 2025
*/


#include <uart.h>
#include <arm_mmu.h>

#include <kernel/mmu.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

// Number of entries in each table
#define NUM_ENTRIES_PER_TABLE 512

// Alignment for the translation table (must be 4KB)
#define TABLE_ALIGNMENT 4096

/* Physical memory regions to map */
#define PHYS_RAM_START        0x40000000 // Typical start address
#define PHYS_RAM_SIZE         0x80000000 // 2GB of RAM
#define PHYS_DEVICE_START     0x00000000 // Device memory start
#define PHYS_DEVICE_SIZE      0x40000000 // Device Memory Size (1GB)

/* Level 1 translation table - statically allocated and aligned */
static uint64_t level1_table[NUM_ENTRIES_PER_TABLE] __attribute__((aligned(TABLE_ALIGNMENT)));

/* Level 2 translation table - one for RAM, one for device */
static uint64_t level2_ram_table[NUM_ENTRIES_PER_TABLE] __attribute__((aligned(TABLE_ALIGNMENT)));
static uint64_t level2_device_table[NUM_ENTRIES_PER_TABLE] __attribute__((aligned(TABLE_ALIGNMENT)));

/* Level 3 tables - for 4KB pages */
static uint64_t level3_tables[4][NUM_ENTRIES_PER_TABLE] __attribute__((aligned(TABLE_ALIGNMENT)));
static int next_l3_table = 0;

static inline uint64_t create_level1_table_entry(uint64_t table_addr)
{
  return (table_addr & PTE_TABLE_ADDR_MASK) | PTE_TYPE_TABLE;
}

/**
 * @brief Create a level2 block entry for memory mapping
 * 
 * @param phys_addr Physical address to map
 * @param attr_index Memory attribute index
 * @param access_perm Access permissions
 * @param shearable Shearable attribute
 * @param execute Wheather the memory is executable
 * @return uint64_t Entry value to be stored in the level 2 table
 * 
 * @author Fedi Nabli
 * @date 14 Mar 2025
 */
static inline uint64_t create_level2_block_entry(
  uint64_t phys_addr,
  uint64_t attr_index,
  uint64_t access_perm,
  uint64_t shearable,
  bool execute
) {
  uint64_t entry = (phys_addr & PTE_BLOCK_ADDR_MASK) | PTE_TYPE_BLOCK | PTE_ATTR_AF;

  // Add memory attributes
  entry |= PTE_ATTR_ATTR_INDX(attr_index);

  // Add access permissions
  entry |= access_perm;

  // Add shearable attributes
  entry |= shearable;

  // Add execute never attributes if needed
  if (!execute)
  {
    entry |= PTE_ATTR_UXN | PTE_ATTR_PXN;
  }

  return entry;
}

/**
 * @brief Create a level3 page entry object
 * 
 * @param phys_addr Physical address to map
 * @param attr_index Memory attribute index
 * @param access_perm Access permissions
 * @param shareable Shearable attribute
 * @param execute Wheather the memory is executable
 * @return uint64_t Entry value to be stored in the level 3 table
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
static inline uint64_t create_level3_page_entry(
  uint64_t phys_addr,
  uint64_t attr_index,
  uint64_t access_perm,
  uint64_t shareable,
  bool execute
) {
  uint64_t entry = (phys_addr & PTE_BLOCK_ADDR_MASK) | PTE_TYPE_PAGE | PTE_ATTR_AF;

  // Add memory attributes
  entry |= PTE_ATTR_ATTR_INDX(attr_index);

  // Add access permissions
  entry |= access_perm;

  // Add shareable attributes
  entry |= shareable;

  // Add execute never attribute if needed
  if (!execute)
  {
    entry |= PTE_ATTR_UXN | PTE_ATTR_PXN;
  }

  return entry;
}

/**
 * @brief Initalize level 1 translation table
 * 
 * Sets up entries in the level 1 table to point to level 2 tables
 * 
 * @author Fedi Nabli
 * @date 14 Mar 2025
 */
static void init_level1_table()
{
  // Clear the table
  for (int i = 0; i < NUM_ENTRIES_PER_TABLE; i++)
  {
    level1_table[i] = 0;
  }

  // Create level 1 entry for RAM region (TTBR0, lower addresses)
  uint64_t ram_table_index = (PHYS_RAM_START >> 30) & 0x1FF; // VA[38:30] for 1GB blocks
  level1_table[ram_table_index] = create_level1_table_entry((uint64_t)level2_ram_table);

  // Create level 2 entry for device region (TTBR0, lower addresses)
  uint64_t device_table_index = (PHYS_DEVICE_START >> 30) & 0x1FF; // VA[38:30] for 1GB blocks
  level1_table[device_table_index] = create_level1_table_entry((uint64_t)level2_device_table);

  uart_send_string("Level 1 table initialized\n");
}

/**
 * @brief Initializes level 2 translation table for RAM
 * 
 * Sets up 2MB block entries for RAM with normal memory attributes
 * 
 * @author Fedi Nabli
 * @date 14 Mar 2025
 */
static void init_level2_ram_table()
{
  // Clear the table
  for (int i = 0; i < NUM_ENTRIES_PER_TABLE; i++)
  {
    level2_ram_table[i] = 0;
  }

  // Create 2MB block entries for RAM (identity mapping)
  uint64_t start_addr = PHYS_RAM_START;
  uint64_t end_addr = PHYS_RAM_START + PHYS_RAM_SIZE;

  for (uint64_t addr = start_addr; addr < end_addr; addr += 0x200000)
  {
    uint64_t index = (addr >> 21) & 0x1FF; // VA[29:21] for 2MB blocks

    // RAM is normal writeback cacheable, inner shearable, RW for EL1, executable
    level2_ram_table[index] = create_level2_block_entry(
      addr, // Physical address
      MEMORY_ATTR_NORMAL_WB, // Memory attribute index
      PTE_ATTR_AP_RW_EL1, // Access permissions
      PTE_ATTR_SH_INNER, // Shearable attribute
      true // Executable
    );
  }

  uart_send_string("Level 2 RAM table initialized\n");
}

/**
 * @brief Initializes level 2 translation table for device memory
 * 
 * Sets up 2MB block entries for device memory with device memory attributes
 * 
 * @author Fedi Nabli
 * @date 14 Mar 2025
 */
static void init_level2_device_table()
{
  // Clear the table
  for (int i = 0; i < NUM_ENTRIES_PER_TABLE; i++)
  {
    level2_device_table[i] = 0;
  }

  // Create 2MB block entries for device memory (identity mapping)
  uint64_t start_addr = PHYS_DEVICE_START;
  uint64_t end_addr = PHYS_DEVICE_START + PHYS_DEVICE_SIZE;

  for (uint64_t addr = start_addr; addr < end_addr; addr += 0x200000)
  {
    uint64_t index = (addr >> 21) & 0x1FF; // VA[29:21] for 2MB block

    // Device memory is nGnRnE, non-shearable, RW for EL1, non-executable
    level2_device_table[index] = create_level2_block_entry(
      addr, // Physical address
      MEMORY_ATTR_DEVICE_nGnRnE, // Memory attribute index from kernel/mmu.h
      PTE_ATTR_AP_RW_EL1, // Access permissions
      PTE_ATTR_SH_NON, // Non-shearable
      false // Non-executable
    );
  }

  uart_send_string("Level 2 device table initialized\n");
}

/**
 * @brief Check if an address can use a large page
 * 
 * @param vaddr Virtual address to check
 * @param paddr Physical address to check
 * @param size Size of the region
 * @param page_size Size of the page
 * @return true if the address can use a large page, false otherwise
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
static bool can_use_large_page(uint64_t vaddr, uint64_t paddr, size_t size, size_t page_size)
{
  // Check alignment for both virtual and physical address
  if ((vaddr & (page_size - 1)) != 0 || (paddr & (page_size - 1)) != 0)
  {
    return false;
  }

  // Check if size is large enough
  return size >=  page_size;
}

/**
 * @brief Get the or create level3 table for a given level 2 entry
 * 
 * @param l2_entry The level 2 entry to create the l3 table for
 * @return uint64_t* Null if error, pointer to the level3 table
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
static uint64_t* get_or_create_level3_table(uint64_t l2_entry)
{
  // If entry is already a table, return it
  if ((l2_entry & PTE_TYPE_MASK) == PTE_TYPE_TABLE)
  {
    return (uint64_t*)(l2_entry & PTE_TABLE_ADDR_MASK);
  }

  // If entry exists but is not a table, we can't proceed
  if ((l2_entry & PTE_TYPE_MASK) != 0)
  {
    return NULL;
  }

  // Use the next available L3 table
  if (next_l3_table >= 4)
  {
    uart_send_string("ERROR: No more L3 tables available");
    return NULL;
  }

  uint64_t* table = level3_tables[next_l3_table++];

  // Clear the table
  for (int i = 0; i < NUM_ENTRIES_PER_TABLE; i++)
  {
    table[i] = 0;
  }

  return table;
}

/**
 * @brief Setup inital memory mappings
 * 
 * This function sets up the initial memory mappings for the kernel.
 * It creates identity mappings for RAM and device memory regions.
 * 
 * @author Fedi Nabli
 * @date 13 Mar 2025
 */
void mmu_setup_initial_mappings()
{
  uart_send_string("Setting up initial memory mapping...\n");

  // Initalize translation tables
  init_level1_table();
  init_level2_ram_table();
  init_level2_device_table();

  // Set translation table base register
  uart_send_string("Setting TTBR0_EL1\n");
  write_ttbr0_el1((uint64_t)level1_table);

  // Invalidate TLB
  invalidate_tlb();

  uart_send_string("Initial memory mappings setup complete\n");
}

/**
 * @brief Initializes the MMU
 * 
 * The function initializes the MMU by:
 * 1. Configuring MMU-related system registers
 * 2. Seeting up initial memory mappings
 * 
 * @warning This function does NOT enable the MMU. Call mmu_enable() seperatly.
 * 
 * @author Fedi Nabli
 * @date 13 Mar 2025
 */
void mmu_init()
{
  uart_send_string("Initializing MMU...\n");

  // Configure MMU-related system registers
  uart_send_string("Configuring MMU registers...\n");
  configure_mair_el1();
  configure_tcr_el1();
  configure_sctlr_el1();

  // Setup initial memory mappings
  mmu_setup_initial_mappings();

  uart_send_string("MMU initalization complete\n");
}

/**
 * @brief Enable the MMU
 * 
 * This function enables the MMU by setting the M bit in SCTLR_EL1
 * 
 * @return 0 on success, non-zero on failure 
 * 
 * @author Fedi Nabli
 * @date 14 Mar 2025
 */
int mmu_enable()
{
  uint64_t sctlr = read_sctlr_el1();
  sctlr |= SCTLR_EL1_M; // Set MMU enable bit

  uart_send_string("Enabling MMU...\n");

  // Write back the updated value
  write_sctlr_el1(sctlr);

  // Ensure the MMU is enabled
  dsb_sy();
  isb_sy();

  // Verify MMU is enabled
  if (!(read_sctlr_el1() & SCTLR_EL1_M))
  {
    uart_send_string("ERROR: Failed to enable MMU\n");
    return -EMMU;
  }

  uart_send_string("MMU enabled successfully\n");
  return EOK;
}

/**
 * @brief Disables the MMU
 * 
 * This function disables the MMU by clearig the M bit in SCTLR_EL1
 * 
 * @return 0 on success, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 14 Mar 2025
 */
int mmu_disable()
{
  uart_send_string("Disabling MMU...\n");

  uint64_t sctlr = read_sctlr_el1();
  sctlr &= ~SCTLR_EL1_M; // Clean MMU enable bit

  // Write back the updated value
  write_sctlr_el1(sctlr);

  // Ensure the MMU disable takes eddect with barriers
  dsb_sy();
  isb_sy();

  // Verify MMU is disabled
  if (read_sctlr_el1() & SCTLR_EL1_M)
  {
    uart_send_string("ERROR: Failed to disable MMU\n");
    return -EMMU;
  }

  uart_send_string("MMU disabled successflly\n");
  return EOK;
}

/**
 * @brief Maps a physical address to a virtual address
 * 
 * This is a basic implementation that will be expanded when implementing paging.
 * Currently just ensures the physical address is covered by our identity mapping.
 * 
 * @param vaddr Virtual address to map to
 * @param paddr Physical address to map from
 * @param size Size of the region to map to
 * @param attrs Memory attributes for the mapping
 * @return 0 on success, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 14 Mar 2025 - Modified 17 Mar 2025
 */
int mmu_map(uint64_t vaddr, uint64_t paddr, uint64_t size, uint64_t attrs)
{
  // Ensure addresses are page-aligned
  if ((vaddr & (PAGE_SIZE - 1)) || (paddr & (PAGE_SIZE - 1)))
  {
    uart_send_string("ERROR: Addresses must be page-aligned");
    return -EINVAL;
  }

  // Map the region
  uint64_t end_addr = vaddr + size;
  uint64_t curr_vaddr = vaddr;
  uint64_t curr_paddr = paddr;

  while (curr_vaddr < end_addr)
  {
    // Get indices for each level
    uint64_t l1_idx = (curr_vaddr >> 30) & 0x1FF;
    uint64_t l2_idx = (curr_vaddr >> 21) & 0x1FF;
    uint64_t l3_idx = (curr_vaddr >> 12) & 0x1FF;

    // Get L1 entry
    uint64_t l1_entry = level1_table[l1_idx];

    // If L1 entry is not a table, we need to create it
    if ((l1_entry & PTE_TYPE_MASK) != PTE_TYPE_TABLE)
    {
      uart_send_string("ERROR: L1 entry is not a table\n");
      return -EINVAL;
    }

    // Get L2 entry
    uint64_t* l2_table = (uint64_t*)(l1_entry & PTE_TABLE_ADDR_MASK);
    uint64_t remaining = end_addr - curr_vaddr;

    // Try to use 2MB block if possible
    if (can_use_large_page(curr_vaddr, curr_paddr, remaining, 0x200000))
    {
      // Create 2MB block entry
      l2_table[l2_idx] = create_level2_block_entry(
        curr_paddr,
        (attrs >> 2) & 0x7, // Extract attribute index
        attrs & 0x300, // Extract access permissions
        attrs & 0xC00, // Extract Shareability
        !(attrs & (PTE_ATTR_UXN | PTE_ATTR_PXN)) // Extract execute permissions
      );

      curr_vaddr += 0x200000;
      curr_paddr += 0X200000;
      
      continue;
    }

    // Otherwise, use 4KB page
    // Get or create L3 table
    uint64_t l2_entry = l2_table[l2_idx];
    uint64_t* l3_table;

    if ((l2_entry & PTE_TYPE_MASK) == PTE_TYPE_TABLE)
    {
      // L3 table already exists
      l3_table = (uint64_t*)(l2_entry & PTE_TABLE_ADDR_MASK);
    }
    else
    {
      // Need to create L3 table
      l3_table = get_or_create_level3_table(l2_entry);
      if (!l3_table)
      {
        uart_send_string("ERROR: Failed to get/create L3 table\n");
        return -ENOMEM;
      }

      // Update L2 entry to point to L3 table
      l2_table[l2_idx] = create_level1_table_entry((uint64_t)l3_table);
    }

    // Create 4KB page entry
    l3_table[l3_idx] = create_level3_page_entry(
      curr_paddr,
      (attrs >> 2) & 0x7, // Extract attribute index
      attrs & 0x300, // Extract access permissions
      attrs & 0xC00, // Extract shareability
      !(attrs & (PTE_ATTR_UXN | PTE_ATTR_PXN)) //Extract execute permissions
    );

    curr_vaddr += PAGE_SIZE;
    curr_paddr += PAGE_SIZE;
  }

  // Invalidate TLB to ensure changes take effect
  invalidate_tlb();

  return EOK;
}

/**
 * @brief Unmaps a virtual address
 * 
 * @param vaddr Virtual address to unmap
 * @param size Size of the region to unmap
 * @return 0 on success, non-zero on failure
 * 
 * @author Fedi Nabli
 * @date 14 Mar 2025 - Modified 17 Mar 2025
 */
int mmu_unmap(uint64_t vaddr, uint64_t size)
{
  if (vaddr & (PAGE_SIZE - 1))
  {
    uart_send_string("ERROR: Address must be page-aligned\n");
    return -EINVAL;
  }

  // Unmap the region
  uint64_t end_addr = vaddr + size;
  uint64_t curr_addr = vaddr;

  while (curr_addr < end_addr)
  {
    // Get indices for each level
    uint64_t l1_idx = (curr_addr >> 30) & 0x1FF;
    uint64_t l2_idx = (curr_addr >> 21) & 0x1FF;
    uint64_t l3_idx = (curr_addr >> 12) & 0x1FF;

    // Get L1 Entry
    uint64_t l1_entry = level1_table[l1_idx];

    // If l1 entry is not a table, address is not mapped
    if ((l1_entry & PTE_TYPE_MASK) != PTE_TYPE_TABLE)
    {
      // Skip to next 1GB boundary
      curr_addr = (curr_addr & ~(0x40000000 - 1)) + 0x40000000;
      continue;
    }

    // Get L2 table
    uint64_t* l2_table = (uint64_t*)(l1_entry & PTE_TABLE_ADDR_MASK);
    uint64_t l2_entry = l2_table[l2_idx];
    uint64_t l2_type = l2_entry & PTE_TYPE_MASK;

    // Check if L2 entry is a block (2MB)
    if (l2_type == PTE_TYPE_BLOCK)
    {
      // If we're unmapping the entire block and it's aligned
      uint64_t remaining = end_addr - curr_addr;
      if (can_use_large_page(curr_addr, 0, remaining, 0x200000))
      {
        // Clear tge entry
        l2_table[l2_idx] = 0;
        curr_addr += 0x200000;
      }
      else
      {
        // We need to split the block
        // For now just skip this block
        uart_send_string("WARNING: Skipping partial 2MB block unmap\n");
        curr_addr = (curr_addr & ~(0x200000 - 1)) + 0x200000;
      }

      continue;
    }

    // If L2 entry is not a table, address is not mapped
    if (l2_type != PTE_TYPE_TABLE)
    {
      // Skip to next 2MB boundary
      curr_addr = (curr_addr & ~(0x200000 - 1)) + 0x200000;
      continue;
    }

    // Get L3 table
    uint64_t* l3_table = (uint64_t*)(l2_entry & PTE_TABLE_ADDR_MASK);

    // Clear the L3 entry (4KB page)
    l3_table[l3_idx] = 0;
    curr_addr += PAGE_SIZE;
  }

  // Invalidate TLB to ensure changes take effect
  invalidate_tlb();

  return EOK;
}

/**
 * @brief Get the Physical address for a virtual address
 * 
 * @param vaddr Virtual address to translate
 * @return uint64_t Physical address, or 0 if not mapped
 * 
 * @author Fedi Nabli
 * @date 17 Mar 2025
 */
uint64_t mmu_virt_to_phys(uint64_t vaddr)
{
  // Get indices for each level
  uint64_t l1_idx = (vaddr >> 30) & 0x1FF;
  uint64_t l2_idx = (vaddr >> 21) & 0x1FF;
  uint64_t l3_idx = (vaddr >> 12) & 0x1FF;

  // Get L1 entry
  uint64_t l1_entry = level1_table[l1_idx];

  // If L1 entry is not a table, not mapped
  if ((l1_entry & PTE_TYPE_MASK) != PTE_TYPE_TABLE)
  {
    return 0;
  }

  // Get L2 table
  uint64_t* l2_table = (uint64_t*)(l1_entry & PTE_TABLE_ADDR_MASK);
  uint64_t l2_entry = l2_table[l2_idx];
  uint64_t l2_type = l2_entry & PTE_TYPE_MASK;

  // Check if L2 entry is a block
  if (l2_type == PTE_TYPE_BLOCK)
  {
    uint64_t offset = vaddr & (0x200000 - 1);
    return (l2_entry & PTE_BLOCK_ADDR_MASK) + offset;
  }

  // If L2 entry is not a table, not mapped
  if (l2_type != PTE_TYPE_TABLE)
  {
    return 0;
  }

  // Get L3 table
  uint64_t* l3_table = (uint64_t*)(l2_entry & PTE_TABLE_ADDR_MASK);
  uint64_t l3_entry = l3_table[l3_idx];

  // Check if L3 entry is a page
  if ((l3_entry & PTE_TYPE_MASK) == PTE_TYPE_PAGE)
  {
    uint64_t offset = vaddr & (PAGE_SIZE - 1);
    return (l3_entry & PTE_BLOCK_ADDR_MASK) + offset;
  }

  return 0;
}
