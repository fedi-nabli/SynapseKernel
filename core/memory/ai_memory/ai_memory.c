#include "ai_memory.h"

#include <uart.h>

#include <kernel/config.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

#include <synapse/memory/memory.h>
#include <synapse/memory/heap/kheap.h>

// Memory pool structure
typedef struct
{
  void* base_addr; // Base address of the pool
  size_t total_size; // Total size of the pool
  size_t used_size; // Used size within the pool

  // Simple free list allocator
  void** free_blocks; // Array of free block pointers
  size_t* free_block_sizes; // Array of free block sizes
  size_t free_block_count; // Number of free blocks

  // Small block allocator for tensor
  void* small_block_pool; // Pool for small tensors
  uint64_t* small_block_bitmap; // Bitmap for small block allocations
  size_t small_block_count; // Number of small blocks

  // Statistics
  size_t allocations; // Total number of allocations
  size_t deallocations; // Total number of deallocations
  size_t peak_usage; // Peak memory usage
} ai_memory_pool_t;

// Global memory pool
static ai_memory_pool_t ai_mem_pool;

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
 * @brief Calculate tensor strides
 * 
 * @param tensor Pointer to the tensor structure
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2021
 */
static void calculate_strides(tensor_t* tensor)
{
  uart_send_string("calculate_strides: Start\n");
  
  if (!tensor || !tensor->shape || tensor->ndim == 0)
  {
    uart_send_string("calculate_strides: Invalid parameters\n");
    return;
  }

  // Allocate strides array if not already allocated
  if (!tensor->strides)
  {
    uart_send_string("calculate_strides: Allocating strides array\n");
    tensor->strides = (size_t*)kmalloc(tensor->ndim * sizeof(size_t));
    if (!tensor->strides)
    {
      uart_send_string("calculate_strides: Allocation failed\n");
      return;
    }
  }

  // Very simple stride calculation:
  // For 1D tensor
  if (tensor->ndim == 1)
  {
    uart_send_string("calculate_strides: 1D tensor\n");
    tensor->strides[0] = 1;
  }
  // For 2D tensor (row-major)
  else if (tensor->ndim == 2)
  {
    uart_send_string("calculate_strides: 2D tensor\n");
    if (tensor->strides && tensor->shape) {
      tensor->strides[1] = 1;                // Last dimension has stride 1
      tensor->strides[0] = tensor->shape[1]; // First dimension has stride = size of second dimension
      uart_send_string("calculate_strides: 2D strides calculated successfully\n");
    } else {
      uart_send_string("calculate_strides: NULL pointer for strides or shape\n");
    }
  }
  // For any higher dimensions, just use a basic pattern (less efficient but safer)
  else
  {
    uart_send_string("calculate_strides: Higher dimension tensor\n");
    tensor->strides[tensor->ndim-1] = 1;  // Last dimension has stride 1
    
    // Work backwards for other dimensions
    for (int i = tensor->ndim - 2; i >= 0; i--)
    {
      tensor->strides[i] = tensor->strides[i+1] * tensor->shape[i+1];
    }
  }
  
  uart_send_string("calculate_strides: Done\n");
}

/**
 * @brief Get optimal alignment for tensor type
 * 
 * @param dtype Tensor data type
 * @return size_t Optimal alignment size in bytes
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
static size_t ai_memory_get_optimal_alignment(tensor_dtype_t dtype)
{
  switch (dtype)
  {
    case TENSOR_TYPE_INT8:
      return 16; // 16-byte alignment for 128-bit SIMD
    case TENSOR_TYPE_INT16:
    case TENSOR_TYPE_FLOAT16:
      return 16; // 16-byte alignment for 128-bit SIMD
    case TENSOR_TYPE_INT32:
    case TENSOR_TYPE_FLOAT32:
      return 32; // 32-byte alignment for 256-bit SIMD
    default:
      return 8; // Default to 8-byte alignment
  }
}

/**
 * @brief Get element size for tensor type
 * 
 * @param dtype Tensor data type
 * @return size_t Element size in bytes
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
static size_t ai_tensor_get_elem_size(tensor_dtype_t dtype)
{
  switch (dtype)
  {
    case TENSOR_TYPE_INT8:
      return 1;
    case TENSOR_TYPE_INT16:
    case TENSOR_TYPE_FLOAT16:
      return 2;
    case TENSOR_TYPE_INT32:
    case TENSOR_TYPE_FLOAT32:
      return 4;
    default:
      return 0;
  }
}

/**
 * @brief Helper to align a pointer
 * 
 * @param ptr Pointer to be aligned
 * @param alignment Alignment boundary
 * @return void* Aligned pointer
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
static void* align_pointer(void* ptr, size_t alignment)
{
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
  return (void*)aligned;
}

/**
 * @brief Small block allocation
 * 
 * @param size Size of the block to allocate
 * @return void* Pointer to the allocated block, or NULL if allocation failed
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
static void* alloc_small_block(size_t size)
{
  // Find a free small block
  for (size_t i = 0; i < ai_mem_pool.small_block_count; i++)
  {
    size_t byte_idx = i / 64;
    size_t bit_idx = i % 64;

    if (!(ai_mem_pool.small_block_bitmap[byte_idx] & (1ULL << bit_idx)))
    {
      // Mark block as used
      ai_mem_pool.small_block_bitmap[byte_idx] |= (1ULL << bit_idx);

      // Calculate block address
      void* block_addr = (void*)((uintptr_t)ai_mem_pool.small_block_pool + (i * AI_MEMORY_MIN_BLOCK_SIZE));

      // Update statistics
      ai_mem_pool.used_size += AI_MEMORY_MIN_BLOCK_SIZE;
      ai_mem_pool.allocations++;
      if (ai_mem_pool.used_size > ai_mem_pool.peak_usage)
      {
        ai_mem_pool.peak_usage = ai_mem_pool.used_size;
      }

      return block_addr;
    }
  }

  // No free small blocks
  return NULL;
}

/**
 * @brief Free a small block
 * 
 * @param ptr Pointer to the block to free
 * @return int Status code (EOK on success, -EINVARG on invalid argument)
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
static int free_small_block(void* ptr)
{
  // Check if the pointer is within the small block pool
  uintptr_t pool_start = (uintptr_t)ai_mem_pool.small_block_pool;
  uintptr_t pool_end = pool_start + (ai_mem_pool.small_block_count * AI_MEMORY_MIN_BLOCK_SIZE);
  uintptr_t addr = (uintptr_t)ptr;

  if (addr < pool_start || addr >= pool_end)
  {
    // Not a small block
    return -EINVARG;
  }

  // Calculate block index
  size_t block_idx = (addr - pool_start) / AI_MEMORY_MIN_BLOCK_SIZE;
  size_t byte_idx = block_idx / 64;
  size_t bit_idx = block_idx % 64;

  // Check if the block is actually allocated
  if (!(ai_mem_pool.small_block_bitmap[byte_idx] & (1ULL << bit_idx)))
  {
    // Block not allocated
    return -EINVARG;
  }

  // Mark block as free
  ai_mem_pool.small_block_bitmap[byte_idx] &= ~(1ULL << bit_idx);

  // Update statistics
  ai_mem_pool.used_size -= AI_MEMORY_MIN_BLOCK_SIZE;
  ai_mem_pool.deallocations++;

  return EOK;
}

/**
 * @brief Allocate memory from the pool
 * 
 * @param size Size of the memory to allocate
 * @param alignment Alignment requirement for the allocation
 * @return void* Pointer to the allocated memory, or NULL if allocation failed
 * 
 * @author Fedi Nabli
 * @date 20 Mar 2025
 */
static void* ai_memory_alloc(size_t size, size_t alignment)
{
  if (size == 0)
  {
    return NULL;
  }

  // Round up size to alignment
  size = (size + alignment - 1) & ~(alignment - 1);

  // For a small allocations, use the small block allocator
  if (size <= AI_MEMORY_MIN_BLOCK_SIZE)
  {
    return alloc_small_block(size);
  }

  // Find best-fit free block
  size_t best_idx = (size_t)-1;
  size_t best_size = (size_t)-1;

  for (size_t i = 0; i < ai_mem_pool.free_block_count; i++)
  {
    void* block = ai_mem_pool.free_blocks[i];
    size_t block_size = ai_mem_pool.free_block_sizes[i];

    // Ensure alignment
    void* aligned_block = align_pointer(block, alignment);
    size_t alignment_overhead = (uint64_t)aligned_block - (uintptr_t)block;

    if (block_size >= size + alignment_overhead && block_size < best_size)
    {
      best_idx = i;
      best_size = block_size;
    }
  }

  if (best_idx == (size_t)-1)
  {
    // No suitable block found, allocate from system
    // MODIFIED: Use kmalloc instead of kpage_alloc_contiguous
    size_t alloc_size = size + alignment;
    void* new_block = kmalloc(alloc_size);

    if (!new_block)
    {
      // Out of memory
      return NULL;
    }

    // Align the new block
    void* aligned_block = align_pointer(new_block, alignment);
    // size_t alignment_overhead = (uintptr_t)aligned_block - (uintptr_t)new_block;

    // Update statistics
    ai_mem_pool.used_size += alloc_size;
    ai_mem_pool.allocations++;
    if (ai_mem_pool.used_size > ai_mem_pool.peak_usage)
    {
      ai_mem_pool.peak_usage = ai_mem_pool.used_size;
    }

    return aligned_block;
  }

  // Use the best-fit block
  void* block = ai_mem_pool.free_blocks[best_idx];
  size_t block_size = ai_mem_pool.free_block_sizes[best_idx];

  // Align the block
  void* aligned_block = align_pointer(block, alignment);
  size_t alignment_overhead = (uintptr_t)aligned_block - (uintptr_t)block;

  // Calculate remaining size after allocation
  size_t remaining_size = block_size - (size + alignment_overhead);

  // If we have enough space left, split the block
  if (remaining_size >= AI_MEMORY_MIN_BLOCK_SIZE)
  {
    void* new_free_block = (void*)((uintptr_t)aligned_block + size);

    // Replace the current free block with the new one
    ai_mem_pool.free_blocks[best_idx] = new_free_block;
    ai_mem_pool.free_block_sizes[best_idx] = remaining_size;
  }
  else
  {
    // Use the entire block
    size = block_size - alignment_overhead;

    // Remove the block from free list
    for (size_t i = best_idx; i < ai_mem_pool.free_block_count - 1; i++)
    {
      ai_mem_pool.free_blocks[i] = ai_mem_pool.free_blocks[i + 1];
      ai_mem_pool.free_block_sizes[i] = ai_mem_pool.free_block_sizes[i + 1];
    }

    ai_mem_pool.free_block_count--;
  }

  // Update statistics
  ai_mem_pool.used_size += size + alignment_overhead;
  ai_mem_pool.allocations++;
  if (ai_mem_pool.used_size > ai_mem_pool.peak_usage)
  {
    ai_mem_pool.peak_usage = ai_mem_pool.used_size;
  }

  return aligned_block;
}

/**
 * @brief Free memory back to the pool
 * 
 * @param ptr Pointer to the memory to free
 * @return int Status code (EOK on success, -EINVARG on invalid argument)
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
static int ai_memory_free(void* ptr)
{
  if (!ptr)
  {
    return -EINVARG;
  }

  // Check if it's a small block
  int res = free_small_block(ptr);
  if (res == EOK)
  {
    // Successfully freed a small block
    return EOK;
  }

  // Add to free list if there's space
  if (ai_mem_pool.free_block_count < AI_MEMORY_MAX_BLOCKS)
  {
    // We don't know the true size, but we'll just use a conservative estimate
    size_t block_size = PAGE_SIZE; // Assume one page for now

    ai_mem_pool.free_blocks[ai_mem_pool.free_block_count] = ptr;
    ai_mem_pool.free_block_sizes[ai_mem_pool.free_block_count] = block_size;
    ai_mem_pool.free_block_count++;

    // Update statistics
    ai_mem_pool.used_size -= block_size;
    ai_mem_pool.deallocations++;

    return EOK;
  }

  // Free list is full, use kfree directly
  // MODIFIED: Use kfree instead of kpage_free
  kfree(ptr);

  // Update statistics (approximate)
  ai_mem_pool.used_size -= PAGE_SIZE; // Assume one page size
  ai_mem_pool.deallocations++;

  return EOK;
}

/**
 * @brief Initialize the AI memory subsystem
 * 
 * @param pool_size Size of the memory pool to allocate
 * @return int Status code (EOK on success, -ENOMEM on allocation failure)
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
int ai_memory_init(size_t pool_size)
{
  // Start with a more reasonable pool size - 1 MB
  size_t requested_pool_size = 1 * 1024 * 1024; // 1 MB
  
  uart_send_string("Initializing AI memory subsystem with ");
  uart_send_string(uint_to_str(requested_pool_size / 1024));
  uart_send_string(" KB pool (using kernel heap)...\n");

  // Clean pool structure
  memset(&ai_mem_pool, 0, sizeof(ai_memory_pool_t));
  ai_mem_pool.total_size = requested_pool_size;

  // Allocate management structures
  ai_mem_pool.free_blocks = (void**)kmalloc(AI_MEMORY_MAX_BLOCKS * sizeof(void*));
  if (!ai_mem_pool.free_blocks)
  {
    uart_send_string("Failed to allocate free block array\n");
    return -ENOMEM;
  }

  ai_mem_pool.free_block_sizes = (size_t*)kmalloc(AI_MEMORY_MAX_BLOCKS * sizeof(size_t));
  if (!ai_mem_pool.free_block_sizes)
  {
    kfree(ai_mem_pool.free_blocks);
    uart_send_string("Failed to allocate free block sizes array\n");
    return -ENOMEM;
  }

  // Calculate small block pool size (1/4 of total)
  size_t small_pool_size = requested_pool_size / 4; 
  small_pool_size = (small_pool_size / PAGE_SIZE) * PAGE_SIZE;

  uart_send_string("Small block pool size: ");
  uart_send_string(uint_to_str(small_pool_size / 1024));
  uart_send_string(" KB\n");

  // Setup small block management
  ai_mem_pool.small_block_count = small_pool_size / AI_MEMORY_MIN_BLOCK_SIZE;
  size_t bitmap_size = (ai_mem_pool.small_block_count + 63) / 64 * sizeof(uint64_t);

  ai_mem_pool.small_block_bitmap = (uint64_t*)kmalloc(bitmap_size);
  if (!ai_mem_pool.small_block_bitmap)
  {
    kfree(ai_mem_pool.free_block_sizes);
    kfree(ai_mem_pool.free_blocks);
    uart_send_string("Failed to allocate small block bitmap\n");
    return -ENOMEM;
  }

  // Clear bitmap
  memset(ai_mem_pool.small_block_bitmap, 0, bitmap_size);
  
  // MODIFIED: Use kmalloc instead of page allocator for small block pool
  uart_send_string("Allocating small block pool using kmalloc...\n");
  
  // Allocate small block pool using kmalloc
  ai_mem_pool.small_block_pool = kmalloc(small_pool_size);
  if (!ai_mem_pool.small_block_pool)
  {
    uart_send_string("Failed to allocate small block pool, trying smaller size...\n");
    
    // Try a smaller allocation
    small_pool_size = PAGE_SIZE * 4; // Try just 16KB
    ai_mem_pool.small_block_pool = kmalloc(small_pool_size);
    
    if (!ai_mem_pool.small_block_pool)
    {
      uart_send_string("Critical failure: Cannot allocate small block pool\n");
      kfree(ai_mem_pool.small_block_bitmap);
      kfree(ai_mem_pool.free_block_sizes);
      kfree(ai_mem_pool.free_blocks);
      return -ENOMEM;
    }
    
    // Adjust small block count
    ai_mem_pool.small_block_count = small_pool_size / AI_MEMORY_MIN_BLOCK_SIZE;
  }
  
  // Clear the small block pool
  memset(ai_mem_pool.small_block_pool, 0, small_pool_size);
  
  uart_send_string("Small block pool allocated at: 0x");
  uart_send_string(uint_to_str((uintptr_t)ai_mem_pool.small_block_pool));
  uart_send_string("\n");
  
  // MODIFIED: Use kmalloc for larger blocks too
  // Allocate a few larger blocks for the free list
  size_t remaining_size = requested_pool_size - small_pool_size;
  size_t block_size = 64 * 1024; // 64KB blocks
  size_t num_blocks = remaining_size / block_size;
  
  // Limit to reasonable number
  if (num_blocks > AI_MEMORY_MAX_BLOCKS - 1)
    num_blocks = AI_MEMORY_MAX_BLOCKS - 1;
  
  uart_send_string("Allocating ");
  uart_send_string(uint_to_str(num_blocks));
  uart_send_string(" larger blocks...\n");
  
  size_t blocks_allocated = 0;
  for (size_t i = 0; i < num_blocks; i++)
  {
    void* block = kmalloc(block_size);
    if (block)
    {
      ai_mem_pool.free_blocks[ai_mem_pool.free_block_count] = block;
      ai_mem_pool.free_block_sizes[ai_mem_pool.free_block_count] = block_size;
      ai_mem_pool.free_block_count++;
      blocks_allocated++;
    }
    else
    {
      break;
    }
  }
  
  uart_send_string("Successfully allocated ");
  uart_send_string(uint_to_str(blocks_allocated));
  uart_send_string(" larger blocks\n");
  
  // Update total size based on what we actually allocated
  ai_mem_pool.total_size = small_pool_size + (blocks_allocated * block_size);
  
  uart_send_string("AI memory subsystem initialized with ");
  uart_send_string(uint_to_str(ai_mem_pool.total_size / 1024));
  uart_send_string(" KB total capacity\n");
  
  // Print some stats
  ai_memory_print_stats();

  return EOK;
}

/**
 * @brief Create a tensor with the given shape and type
 * 
 * @param shape Pointer to the shape array
 * @param ndim Number of dimensions
 * @param dtype Data type of the tensor elements
 * @param layout Memory layout of the tensor
 * @param flags Allocation flags
 * @return tensor_t* Pointer to the created tensor, or NULL on failure
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
tensor_t* ai_tensor_create(size_t* shape, size_t ndim, tensor_dtype_t dtype, tensor_layout_t layout, uint32_t flags)
{
  uart_send_string("ai_tensor_create: Starting tensor creation\n");
  
  if (!shape || ndim == 0 || dtype >= TENSOR_TYPE_COUNT)
  {
    uart_send_string("ai_tensor_create: Invalid parameters\n");
    return NULL;
  }

  // Calculate total elements and memory size
  size_t total_elems = 1;
  for (size_t i = 0; i < ndim; i++)
  {
    uart_send_string("Calculating elements: shape[");
    uart_send_string(uint_to_str(i));
    uart_send_string("] = ");
    uart_send_string(uint_to_str(shape[i]));
    uart_send_string("\n");

    total_elems *= shape[i];
  }

  uart_send_string("ai_tensor_create: Total elements: ");
  uart_send_string(uint_to_str(total_elems));
  uart_send_string("\n");

  size_t elem_size = ai_tensor_get_elem_size(dtype);
  size_t memory_size = total_elems * elem_size;

  uart_send_string("ai_tensor_create: Element size: ");
  uart_send_string(uint_to_str(elem_size));
  uart_send_string(", Memory size: ");
  uart_send_string(uint_to_str(memory_size));
  uart_send_string("\n");

  // Determine alignment
  size_t alignment = 8; // Default alignment
  if (flags & TENSOR_MEM_ALIGNED)
  {
    alignment = ai_memory_get_optimal_alignment(dtype);
  }

  uart_send_string("ai_tensor_create: Using alignment: ");
  uart_send_string(uint_to_str(alignment));
  uart_send_string("\n");

  // Allocate tensor descriptor
  uart_send_string("ai_tensor_create: Allocating tensor descriptor\n");
  tensor_t* tensor = (tensor_t*)kmalloc(sizeof(tensor_t));
  if (!tensor)
  {
    uart_send_string("ai_tensor_create: Failed to allocate tensor descriptor\n");
    return NULL;
  }

  // Clean tensor
  uart_send_string("ai_tensor_create: Clearing tensor descriptor\n");
  memset(tensor, 0, sizeof(tensor_t));

  // Allocate shape array
  uart_send_string("ai_tensor_create: Allocating shape array\n");
  tensor->shape = (size_t*)kmalloc(ndim * sizeof(size_t));
  if (!tensor->shape)
  {
    uart_send_string("ai_tensor_create: Failed to allocate shape array\n");
    kfree(tensor);
    return NULL;
  }

  // Copy shape
  uart_send_string("ai_tensor_create: Copying shape\n");
  for (size_t i = 0; i < ndim; i++)
  {
    tensor->shape[i] = shape[i];
  }

  // Allocate strides array
  uart_send_string("ai_tensor_create: Allocating strides array\n");
  tensor->strides = (size_t*)kmalloc(ndim * sizeof(size_t));
  if (!tensor->strides)
  {
    uart_send_string("ai_tensor_create: Failed to allocate strides array\n");
    kfree(tensor->shape);
    kfree(tensor);
    return NULL;
  }

  // Set tensor properties
  uart_send_string("ai_tensor_create: Setting tensor properties\n");
  tensor->ndim = ndim;
  tensor->dtype = dtype;
  tensor->elem_size = elem_size;
  tensor->layout = layout;
  tensor->flags = flags;

  // Calculate strides
  uart_send_string("ai_tensor_create: Calculating strides\n");
  calculate_strides(tensor);

  // Use direct kmalloc for data
  uart_send_string("ai_tensor_create: Allocating tensor data with kmalloc\n");
  tensor->data = ai_memory_alloc(memory_size, alignment);
  if (!tensor->data)
  {
    uart_send_string("ai_tensor_create: Failed to allocate tensor data\n");
    kfree(tensor->strides);
    kfree(tensor->shape);
    kfree(tensor);
    return NULL;
  }
  
  uart_send_string("ai_tensor_create: Data allocated at 0x");
  uart_send_string(uint_to_str((uintptr_t)tensor->data));
  uart_send_string("\n");

  // Zero data if requested
  if (flags & TENSOR_MEM_ZEROED)
  {
    uart_send_string("ai_tensor_create: Zeroing tensor data\n");
    memset(tensor->data, 0, memory_size);
  }

  uart_send_string("ai_tensor_create: Tensor creation successful\n");
  return tensor;
}

/**
 * @brief Destroy a tensor and free its memory (debug version with logging)
 * 
 * @param tensor Pointer to the tensor to destroy
 * @return int Status code (EOK on success, -EINVARG on invalid argument)
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
int ai_tensor_destroy(tensor_t* tensor)
{
  uart_send_string("ai_tensor_destroy: Starting tensor destruction\n");
  
  if (!tensor)
  {
    uart_send_string("ai_tensor_destroy: NULL tensor pointer\n");
    return -EINVARG;
  }

  // Free tensor data
  if (tensor->data)
  {
    uart_send_string("ai_tensor_destroy: Freeing tensor data at 0x");
    uart_send_string(uint_to_str((uintptr_t)tensor->data));
    uart_send_string("\n");
    
    ai_memory_free(tensor->data);
  }
  else
  {
    uart_send_string("ai_tensor_destroy: Tensor has NULL data pointer\n");
  }

  // Free shape and strides array
  if (tensor->shape)
  {
    uart_send_string("ai_tensor_destroy: Freeing shape array\n");
    kfree(tensor->shape);
  }

  if (tensor->strides)
  {
    uart_send_string("ai_tensor_destroy: Freeing strides array\n");
    kfree(tensor->strides);
  }

  // Free tensor descriptor
  uart_send_string("ai_tensor_destroy: Freeing tensor descriptor\n");
  kfree(tensor);

  uart_send_string("ai_tensor_destroy: Tensor destruction complete\n");
  return EOK;
}

/**
 * @brief Reshape a tensor without changing its data
 * 
 * @param tensor Pointer to the tensor to reshape
 * @param new_shape Pointer to the new shape array
 * @param new_ndim Number of dimensions in the new shape
 * @return int Status code (EOK on success, -EINVARG on invalid argument, -ENOMEM on memory allocation failure)
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
int ai_tensor_reshape(tensor_t* tensor, size_t* new_shape, size_t new_ndim)
{
  if (!tensor || !new_shape || new_ndim == 0)
  {
    return -EINVARG;
  }

  // Calculate total elements in new shape
  size_t new_total_elems = 1;
  for (size_t i = 0; i < new_ndim; i++)
  {
    new_total_elems *= new_shape[i];
  }

  // Calcilate total elements in current shape
  size_t current_total_elems = 1;
  for (size_t i = 0; i < tensor->ndim; i++)
  {
    current_total_elems *= tensor->shape[i];
  }

  // Verify element count matches
  if (new_total_elems != current_total_elems)
  {
    return -EINVARG;
  }

  // Reallocate shape array if needed
  if (new_ndim != tensor->ndim)
  {
    size_t* new_shape_array = (size_t*)kmalloc(new_ndim * sizeof(size_t));
    if (!new_shape_array)
    {
      return -ENOMEM;
    }

    size_t* new_strides_array = (size_t*)kmalloc(new_ndim * sizeof(size_t));
    if (!new_strides_array)
    {
      kfree(new_shape_array);
      return -ENOMEM;
    }

    // Free old arrays
    kfree(tensor->shape);
    kfree(tensor->strides);

    // Set new array
    tensor->shape = new_shape_array;
    tensor->strides = new_strides_array;
    tensor->ndim = new_ndim;
  }

  // Copy new shape
  for (size_t i = 0; i < new_ndim; i++)
  {
    tensor->shape[i] = new_shape[i];
  }

  // Recalculate strides
  calculate_strides(tensor);

  return EOK;
}

/**
 * @brief Change tensor memory layout
 * 
 * @param tensor Pointer to the tensor to change layout
 * @param new_layout New memory layout for the tensor
 * @return int Status code (EOK on success, -EINVARG on invalid argument)
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
int ai_tensor_set_layout(tensor_t* tensor, tensor_layout_t new_layout)
{
  if (!tensor)
  {
    return -EINVARG;
  }

  // If layout is the same, do nothing
  if (tensor->layout == new_layout)
  {
    return EOK;
  }

  // Set new layout
  tensor->layout = new_layout;

  // Recalculate strides
  calculate_strides(tensor);

  // For a real implementation, we might need to rearrange the data in memory
  // based on the new layout, but this is a simplied version

  return EOK;
}

/**
 * @brief Get tensor memory alignment
 * 
 * @param tensor Pointer to the tensor
 * @return size_t Alignment size in bytes
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
size_t ai_tensor_get_alignment(tensor_t* tensor)
{
  if (!tensor)
  {
    return 0;
  }

  if (tensor->flags & TENSOR_MEM_ALIGNED)
  {
    return ai_memory_get_optimal_alignment(tensor->dtype);
  }

  return 8; // Default alignment
}

/**
 * @brief Get total tensor size in bytes
 * 
 * @param tensor Pointer to the tensor
 * @return size_t Total size of the tensor in bytes
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
size_t ai_tensor_get_size(tensor_t* tensor)
{
  if (!tensor)
  {
    return 0;
  }

  // Calculate total elements
  size_t total_elems = 1;
  for (size_t i = 0; i < tensor->ndim; i++)
  {
    total_elems *= tensor->shape[i];
  }

  return total_elems * tensor->elem_size;
}

/**
 * @brief Get element at specified indices
 * 
 * @param tensor Pointer to the tensor
 * @param indices Array of indices for each dimension
 * @return void* Pointer to the element at the specified indices, or NULL on error
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
void* ai_tensor_get_element(tensor_t* tensor, size_t* indices)
{
  if (!tensor || !indices)
  {
    return NULL;
  }

  // Calculate offset
  size_t offset = 0;
  for (size_t i = 0; i < tensor->ndim; i++)
  {
    offset += indices[i] * tensor->strides[i];
  }

  // Get pointer to element
  return (void*)((uintptr_t)tensor->data + (offset * tensor->elem_size));
}

/**
 * @brief Copy data to tensor
 * 
 * @param tensor Pointer to the tensor
 * @param data Pointer to the data to copy
 * @param size Size of the data to copy in bytes
 * @return int Status code (EOK on success, -EINVARG on invalid argument)
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
int ai_tensor_copy_data(tensor_t* tensor, void* data, size_t size)
{
  if (!tensor || !data)
  {
    return -EINVARG;
  }

  size_t tensor_size = ai_tensor_get_size(tensor);
  if (size > tensor_size)
  {
    size = tensor_size; // Truncate to tensor size
  }

  // Copy data
  memcpy(tensor->data, data, size);

  return EOK;
}

/**
 * @brief Create a view of a tensor (shared data)
 * 
 * @param tensor Pointer to the original tensor
 * @param start_indices Array of start indices for each dimension
 * @param shape Array of shape for the view
 * @return tensor_t* Pointer to the view tensor, or NULL on failure
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
tensor_t* ai_tensor_view(tensor_t* tensor, size_t* start_indices, size_t* shape)
{
  if (!tensor || !start_indices || !shape)
  {
    return NULL;
  }

  // Verify indices are within bounds
  for (size_t i = 0; i < tensor->ndim; i++)
  {
    if (start_indices[i] + shape[i] > tensor->shape[i])
    {
      // Out of bounds
      return NULL;
    }
  }

  // Allocate view tensor
  tensor_t* view = (tensor_t*)kmalloc(sizeof(tensor_t));
  if (!view)
  {
    return NULL;
  }

  // Copy tensor properties
  view->dtype = tensor->dtype;
  view->elem_size = tensor->elem_size;
  view->layout = tensor->layout;
  view->flags = tensor->flags;
  view->ndim = tensor->ndim;

  // Allocate shape and strides array
  view->shape = (size_t*)kmalloc(tensor->ndim * sizeof(size_t));
  if (!view->shape)
  {
    kfree(view);
    return NULL;
  }

  view->strides = (size_t*)kmalloc(tensor->ndim * sizeof(size_t));
  if (!view->strides)
  {
    kfree(view->shape);
    kfree(view);
    return NULL;
  }

  // Copy shape and strides
  for (size_t i = 0; i < tensor->ndim; i++)
  {
    view->shape[i] = shape[i];
    view->strides[i] = tensor->strides[i];
  }

  // Calculate data pointer offset
  size_t offset = 0;
  for (size_t i = 0; i < tensor->ndim; i++)
  {
    offset += start_indices[i] * tensor->strides[i];
  }

  // Set data pointer
  view->data = (void*)((uintptr_t)tensor->data + (offset * tensor->elem_size));

  return view;
}

/**
 * @brief Print AI memory pool statistics
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
void ai_memory_print_stats()
{
  uart_send_string("AI memory pool statistics:\n");
  uart_send_string("  Total size: ");
  uart_send_string(uint_to_str(ai_mem_pool.total_size / 1024));
  uart_send_string(" KB\n  Used size: ");
  uart_send_string(uint_to_str(ai_mem_pool.used_size / 1024));
  uart_send_string(" KB\n  Free size: ");
  uart_send_string(uint_to_str((ai_mem_pool.total_size - ai_mem_pool.used_size) / 1024));
  uart_send_string(" KB\n  Peak usage: ");
  uart_send_string(uint_to_str(ai_mem_pool.peak_usage / 1024));
  uart_send_string(" KB\n  Allocations: ");
  uart_send_string(uint_to_str(ai_mem_pool.allocations));
  uart_send_string("\n  Deallocations: ");
  uart_send_string(uint_to_str(ai_mem_pool.deallocations));
  uart_send_string("\n  Small blocks: ");
  uart_send_string(uint_to_str(ai_mem_pool.small_block_count));
  uart_send_string(" total, ");

  // Count free small blocks
  size_t free_small_blocks = 0;
  for (size_t i = 0; i < ai_mem_pool.small_block_count; i++)
  {
    size_t byte_idx = i / 64;
    size_t bit_idx = i % 64;

    if (!(ai_mem_pool.small_block_bitmap[byte_idx] & (1ULL << bit_idx)))
    {
      free_small_blocks++;
    }
  }

  uart_send_string(uint_to_str(free_small_blocks));
  uart_send_string(" free\n");
}
