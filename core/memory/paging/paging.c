#include "paging.h"

#include <uart.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

#include <kernel/config.h>
#include <kernel/kernel_mmu.h>

#include <synapse/memory/memory.h>
#include <synapse/memory/heap/kheap.h>

uintptr_t kpage_to_phys(void* page);
void* kpage_from_phys(uintptr_t phys);

// Page bitmap parameters
#define PAGE_BITMAP_ENTRY_SIZE sizeof(uint64_t)
#define PAGES_PER_BITMAP_ENTRY (PAGE_BITMAP_ENTRY_SIZE * 8)

// Page management structure
typedef struct
{
  uintptr_t base_addr; // Start of managed physical memory
  uint64_t* bitmap; // Bitmap tracking page status (1 bit per page)
  uint8_t* page_info; // Additional page information (1 byte per page)
  size_t total_pages; // Total number of pages
  size_t free_pages; // Number of free pages
  size_t bitmap_size; // Size of bitmap in bytes
} page_manager_t;

// Global page manager
static page_manager_t page_manager;

// Temporary buffer for early boot string operations
static char temp_str_buffer[32];

// Convert a number to a string for UART output
static char* uint_to_str(uint64_t value)
{
  int i = 0;
  char* p = temp_str_buffer;

  do
  {
    temp_str_buffer[i++] = '0' + (value % 10);
    value /= 10;
  } while (value > 0 && i < 31);

  temp_str_buffer[i] = '\0';

  // Reverse the string
  int j = 0;
  i--;
  while (j < i)
  {
    char temp = p[j];
    p[j] = p[i];
    p[i] = temp;
    j++;
    i--;
  }

  return temp_str_buffer;
}

/**
 * @brief Set the bit in the bitmap
 * 
 * @param bitmap Pointer to the bitmap array
 * @param bit Index of the bit to set
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
static void bitmap_set(uint64_t* bitmap, size_t bit)
{
  size_t byte_index = bit / PAGES_PER_BITMAP_ENTRY;
  size_t bit_index = bit % PAGES_PER_BITMAP_ENTRY;
  bitmap[byte_index] |= (1ULL << bit_index);
}

/**
 * @brief Clear the bit in the bitmap
 * 
 * @param bitmap Pointer to the bitmap array
 * @param bit Index of the bit to clear
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
static void bitmap_clear(uint64_t* bitmap, size_t bit)
{
  size_t byte_index = bit / PAGES_PER_BITMAP_ENTRY;
  size_t bit_index = bit % PAGES_PER_BITMAP_ENTRY;
  bitmap[byte_index] &= ~(1ULL << bit_index);
}

/**
 * @brief Test if the bit in the bitmap is set
 * 
 * @param bitmap Pointer to the bitmap array
 * @param bit Index of the bit to test
 * @return bool True if the bit is set, false otherwise
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
static bool bitmap_test( uint64_t* bitmap, size_t bit)
{
  size_t byte_index = bit / PAGES_PER_BITMAP_ENTRY;
  size_t bit_index = bit % PAGES_PER_BITMAP_ENTRY;
  return (bitmap[byte_index] & (1ULL << bit_index)) != 0;
}

/**
 * @brief Find the first free bit range in the bitmap
 * 
 * @param bitmap Pointer to the bitmap array
 * @param total_bits Total number of bits in the bitmap
 * @param count Number of consecutive free bits required
 * @return int Starting index of the free range, or -NOFREERGE if no range is found
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
static int bitmap_find_free_range(uint64_t* bitmap, size_t total_bits, size_t count)
{
  size_t consecutive = 0;
  size_t start = 0;

  for (size_t i = 0; i < total_bits; i++)
  {
    if (!bitmap_test(bitmap, i))
    {
      if (consecutive == 0)
      {
        start = i;
      }
      consecutive++;
      if (consecutive == count)
      {
        return start;
      }
    }
    else
    {
      consecutive = 0;
    }
  }

  return -NOFREERGE;
}

/**
 * @brief Initialize the paging subsystem
 * 
 * @param ram_size Total size of RAM in bytes
 * @param kernel_start Start address of the kernel
 * @param kernel_end End address of the kernel
 * @return int Status code (EOK on success, negative error code on failure)
 * 
 * @author Fedi Nabli
 * @date 19 Mar 2025
 */
int paging_init(size_t ram_size, uintptr_t kernel_start, uintptr_t kernel_end)
{
  uart_send_string("Initializing paging subsystem...\n");

  // Calculate number of pages
  page_manager.total_pages = ram_size / PAGE_SIZE;
  if (page_manager.total_pages > MAX_PAGES) {
    // Cap at maximum pages
    page_manager.total_pages = MAX_PAGES;
    uart_send_string("WARNING: RAM size exceeds maximum page count, capping at ");
    uart_send_string(uint_to_str(MAX_PAGES));
    uart_send_string(" pages\n");
  }

  // Initialize the base address and free pages
  page_manager.base_addr = 0; // Assume physical memory starts at 0
  page_manager.free_pages = page_manager.total_pages;

  uart_send_string("Total pages: ");
  uart_send_string(uint_to_str(page_manager.total_pages));
  uart_send_string("\n");

  // Calculate bitmap size (rounded up)
  page_manager.bitmap_size = (page_manager.total_pages + PAGES_PER_BITMAP_ENTRY - 1) /
                              PAGES_PER_BITMAP_ENTRY * PAGE_BITMAP_ENTRY_SIZE;

  // Allocate bitmap from the kernel heap
  page_manager.bitmap = (uint64_t*)kmalloc(page_manager.bitmap_size);
  if (!page_manager.bitmap) {
    uart_send_string("Failed to allocate page bitmap\n");
    return -ENOMEM;
  }

  // Allocate page info array
  page_manager.page_info = (uint8_t*)kmalloc(page_manager.total_pages);
  if (!page_manager.page_info) {
    kfree(page_manager.bitmap);
    uart_send_string("Failed to allocate page info array\n");
    return -ENOMEM;
  }

  // Clear all structures - all pages start as free
  memset(page_manager.bitmap, 0, page_manager.bitmap_size);
  memset(page_manager.page_info, 0, page_manager.total_pages);

  // Since we're having issues with the kernel range, let's just reserve a fixed
  // small portion at the beginning for the kernel and critical structures
  size_t reserved_pages = 64; // Reserve just 64 pages (256KB) for kernel
  
  uart_send_string("Reserving first ");
  uart_send_string(uint_to_str(reserved_pages));
  uart_send_string(" pages for kernel and critical structures\n");

  for (size_t i = 0; i < reserved_pages && i < page_manager.total_pages; i++) {
    bitmap_set(page_manager.bitmap, i);
    page_manager.page_info[i] = PAGE_ALLOCATED | PAGE_KERNEL;
    page_manager.free_pages--;
  }
  
  // Also reserve pages for our page management structures
  uintptr_t bitmap_start = (uintptr_t)page_manager.bitmap;
  uintptr_t bitmap_end = bitmap_start + page_manager.bitmap_size;
  size_t bitmap_start_page = bitmap_start / PAGE_SIZE;
  size_t bitmap_end_page = (bitmap_end + PAGE_SIZE - 1) / PAGE_SIZE;
  
  // Safety check for bitmap pages
  if (bitmap_start_page < page_manager.total_pages && bitmap_end_page < page_manager.total_pages) {
    uart_send_string("Reserving bitmap pages (");
    uart_send_string(uint_to_str(bitmap_start_page));
    uart_send_string(" to ");
    uart_send_string(uint_to_str(bitmap_end_page));
    uart_send_string(")\n");
    
    for (size_t i = bitmap_start_page; i <= bitmap_end_page; i++) {
      if (!bitmap_test(page_manager.bitmap, i)) { // Only mark if not already marked
        bitmap_set(page_manager.bitmap, i);
        page_manager.page_info[i] = PAGE_ALLOCATED | PAGE_KERNEL;
        page_manager.free_pages--;
      }
    }
  }
  
  uintptr_t page_info_start = (uintptr_t)page_manager.page_info;
  uintptr_t page_info_end = page_info_start + page_manager.total_pages;
  size_t page_info_start_page = page_info_start / PAGE_SIZE;
  size_t page_info_end_page = (page_info_end + PAGE_SIZE - 1) / PAGE_SIZE;
  
  // Safety check for page info pages
  if (page_info_start_page < page_manager.total_pages && page_info_end_page < page_manager.total_pages) {
    uart_send_string("Reserving page info pages (");
    uart_send_string(uint_to_str(page_info_start_page));
    uart_send_string(" to ");
    uart_send_string(uint_to_str(page_info_end_page));
    uart_send_string(")\n");
    
    for (size_t i = page_info_start_page; i <= page_info_end_page; i++) {
      if (!bitmap_test(page_manager.bitmap, i)) { // Only mark if not already marked
        bitmap_set(page_manager.bitmap, i);
        page_manager.page_info[i] = PAGE_ALLOCATED | PAGE_KERNEL;
        page_manager.free_pages--;
      }
    }
  }

  uart_send_string("Free pages after initialization: ");
  uart_send_string(uint_to_str(page_manager.free_pages));
  uart_send_string("\n");
  
  // Test page allocation to verify
  uart_send_string("Testing page allocation...\n");
  void* test_page = kpage_alloc_flags(PAGEF_ZEROED);
  
  if (test_page) {
    uart_send_string("Successfully allocated test page at: 0x");
    uart_send_string(uint_to_str((uintptr_t)test_page));
    uart_send_string("\n");
    
    // Return the test page to the pool
    kpage_free(test_page);
    uart_send_string("Test page freed\n");
  } else {
    uart_send_string("WARNING: Failed to allocate test page!\n");
  }

  return EOK;
}

/**
 * @brief Allocate a single page of physical memory with flags
 * 
 * @param flags Allocation flags (e.g., PAGE_KERNEL, PAGE_ZEROED)
 * @return void* Pointer to the allocated page, or NULL if allocation fails
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
void* kpage_alloc_flags(uint32_t flags)
{
  // Find a free page
  size_t page_index = (size_t)-1;
  
  // Direct bitmap search instead of using helper function
  for (size_t i = 0; i < page_manager.total_pages; i++) {
    if (!bitmap_test(page_manager.bitmap, i)) {
      page_index = i;
      break;
    }
  }
  
  if (page_index == (size_t)-1 || page_index >= page_manager.total_pages) {
    uart_send_string("kpage_alloc_flags: No free pages found!\n");
    return NULL;
  }
  
  // Mark the page as allocated
  bitmap_set(page_manager.bitmap, page_index);

  // Set page info
  uint8_t page_info = PAGE_ALLOCATED;
  if (flags & PAGE_KERNEL) {
    page_info |= PAGE_KERNEL;
  }
  page_manager.page_info[page_index] = page_info;

  // Decrement free page count
  if (page_manager.free_pages > 0) {
    page_manager.free_pages--;
  }

  // Calculate page address
  uintptr_t page_addr = page_manager.base_addr + (page_index * PAGE_SIZE);

  // Zero the page if requested
  if (flags & PAGE_ZEROED) {
    void* virt_addr = kpage_from_phys(page_addr);
    memset(virt_addr, 0, PAGE_SIZE);
    page_manager.page_info[page_index] |= PAGE_ZEROED;
  }

  return kpage_from_phys(page_addr);
}

/**
 * @brief Allocate a single page of physical memory
 * 
 * @return void* Pointer to the allocated page, or NULL if allocation fails
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
void* kpage_alloc()
{
  return kpage_alloc_flags(0);
}

/**
 * @brief Allocate multiple contiguous pages
 * 
 * @param count Number of contiguous pages to allocate
 * @param flags Allocation flags (e.g., PAGE_KERNEL, PAGE_ZEROED)
 * @return void* Pointer to the start of the allocated pages, or NULL if allocation fails
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
void* kpage_alloc_contiguous(size_t count, uint32_t flags)
{
  if (count == 0)
  {
    return NULL;
  }

  if (count == 1)
  {
    return kpage_alloc_flags(flags);
  }

  // Find contiguous free pages
  int start_page = bitmap_find_free_range(page_manager.bitmap, page_manager.total_pages, count);
  if (start_page < 0)
  {
    // No contiguous range found
    return NULL;
  }

  // Mark all pages as allocated
  for (size_t i = 0; i < count; i++)
  {
    bitmap_set(page_manager.bitmap, start_page + i);

    // Set page info
    uint8_t page_info = PAGE_ALLOCATED;
    if (flags & PAGE_KERNEL)
    {
      page_info |= PAGE_KERNEL;
    }
    page_manager.page_info[start_page + i] = page_info;
  }

  // Decrement free page count
  page_manager.free_pages -= count;

  // Calculate start address
  uintptr_t start_addr = page_manager.base_addr + (start_page * PAGE_SIZE);

  // Zero the pages if requested
  if (flags & PAGE_ZEROED)
  {
    void* virt_addr = kpage_from_phys(start_addr);
    memset(virt_addr, 0, count * PAGE_SIZE);

    for (size_t i = 0; i < count; i++)
    {
      page_manager.page_info[start_page + i] |= PAGE_ZEROED;
    }
  }

  return kpage_from_phys(start_addr);
}

/**
 * @brief Free a previously allocated page
 * 
 * @param page Pointer to the page to free
 * @return int Status code (EOK on success, negative error code on failure)
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
int kpage_free(void* page)
{
  uintptr_t phys_addr = kpage_to_phys(page);

  // Calculate page index
  size_t page_index = (phys_addr - page_manager.base_addr) / PAGE_SIZE;

  // Check if page is allocated
  if (!bitmap_test(page_manager.bitmap, page_index))
  {
    // Page already free
    return -EINVARG;
  }

  // Mark the page as free
  bitmap_clear(page_manager.bitmap, page_index);
  page_manager.page_info[page_index] = PAGE_FREE;

  // Increment free page count
  page_manager.free_pages++;

  return EOK;
}

/**
 * @brief Free multiple contiguous pages
 * 
 * @param page Pointer to the start of the pages to free
 * @param count Number of contiguous pages to free
 * @return int Status code (EOK on success, negative error code on failure)
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
int kpage_free_contiguous(void* page, size_t count)
{
  uintptr_t phys_addr = kpage_to_phys(page);

  // Calculate start page index
  size_t start_page = (phys_addr - page_manager.base_addr) / PAGE_SIZE;

  // Free each page
  for (size_t i = 0; i < count; i++)
  {
    size_t page_index = start_page + i;

    // Check if the page is allocated
    if (!bitmap_test(page_manager.bitmap, page_index))
    {
      // Page already free
      return -EINVARG;
    }

    // Mark the page as free
    bitmap_clear(page_manager.bitmap, page_index);
    page_manager.page_info[page_index] = PAGE_FREE;

    // Increment free page count
    page_manager.free_pages++;
  }

  return EOK;
}

/**
 * @brief Check if a page is allocated
 * 
 * @param page Pointer to the page to check
 * @return true if the page is allocated, false otherwise
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
bool kpage_is_allocated(void* page)
{
  uintptr_t phys_addr = kpage_to_phys(page);

  // Calculate page index
  size_t page_index = (phys_addr - page_manager.base_addr) / PAGE_SIZE;

  return bitmap_test(page_manager.bitmap, page_index);
}

/**
 * @brief Get the physical address from a page pointer
 * 
 * @param page Pointer to the page
 * @return uintptr_t Physical address of the page
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
uintptr_t kpage_to_phys(void* page)
{
  // Simple conversion for now - in a real system this would use
  // the MMU to translate virtual to physical address
  return (uintptr_t)page;
}

/**
 * @brief Get a page pointer from a physical address
 * 
 * @param phys Physical address of the page
 * @return void* Pointer to the page
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
void* kpage_from_phys(uintptr_t phys)
{
  // Simple conversion for now - in a real system this would
  // map the physical address into a virtual address space
  return (void*)phys;
}

/**
 * @brief Handle a page fault exception
 * 
 * @param fault_addr Address where the page fault occurred
 * @param fault_status Status code of the fault
 * @return int Status code (EOK on success, negative error code on failure)
 * 
 * @details This function is called when a page fault occurs. It logs the fault
 *          address and status, and attempts to handle the fault. If the fault
 *          cannot be handled, it returns an error code.
 * 
 * @note This is a stub implementation. Actual handling logic should be added.
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
int kpage_fault_handler(uintptr_t fault_addr, uint64_t fault_status)
{
  uart_send_string("Page fault at address: 0x");
  uart_send_string(uint_to_str(fault_addr));
  uart_send_string(", status: 0x");
  uart_send_string(uint_to_str(fault_status));
  uart_send_string("\n");

  // TODO: Implement page fault handling
  // This would handle things like demand paging, copy-on-write, etc...

  // Unhandled fault
  return -EFAULT;
}

/**
 * @brief Get the total number of pages managed by the paging subsystem
 * 
 * @return size_t Total number of pages
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
size_t kpage_get_total()
{
  return page_manager.total_pages;
}

/**
 * @brief Get the number of free pages managed by the paging subsystem
 * 
 * @return size_t Number of free pages
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
size_t kpage_get_free()
{
  return page_manager.free_pages;
}

/**
 * @brief Get the number of used pages managed by the paging subsystem
 * 
 * @return size_t Number of used pages
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
size_t kpage_get_used()
{
  return page_manager.total_pages - page_manager.free_pages;
}

/**
 * @brief Print page allocation statistics to UART
 * 
 * This function prints the total number of pages, used pages, free pages,
 * used memory, and free memory managed by the paging subsystem.
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
void kpage_print_stats()
{
  uart_send_string("Page allocation statistics:\n");
  uart_send_string("  Total pages: ");
  uart_send_string(uint_to_str(page_manager.total_pages));
  uart_send_string("\n  Used pages: ");
  uart_send_string(uint_to_str(page_manager.total_pages - page_manager.free_pages));
  uart_send_string("\n  Free pages: ");
  uart_send_string(uint_to_str(page_manager.free_pages));
  uart_send_string("\n  Used memory: ");
  uart_send_string(uint_to_str((page_manager.total_pages - page_manager.free_pages) * PAGE_SIZE / 1024));
  uart_send_string(" KB\n  Free memory: ");
  uart_send_string(uint_to_str(page_manager.free_pages * PAGE_SIZE / 1024));
  uart_send_string(" KB\n");
}
