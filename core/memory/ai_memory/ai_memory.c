#include "ai_memory.h"

#include <uart.h>

#include <kernel/config.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/status.h>

#include <synapse/memory/memory.h>
#include <synapse/memory/heap/kheap.h>
#include <synapse/memory/paging/paging.h>

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
  if (!tensor || !tensor->shape || tensor->ndim == 0)
  {
    return;
  }

  // Allocate strides array if not already allocated
  if (!tensor->strides)
  {
    tensor->strides = (size_t*)kmalloc(tensor->ndim * sizeof(size_t));
    if (!tensor->strides)
    {
      return;
    }
  }

  // Calculate strides based on layout
  switch (tensor->layout)
  {
    case TENSOR_LAYOUT_ROW_MAJOR:
      // Row-major: rightmost dimension changes fastest
      tensor->strides[tensor->ndim - 1] = 1;
      for (int i = tensor->ndim - 2; i >= 0; i--)
      {
        tensor->strides[i] = tensor->strides[i + 1] * tensor->shape[i + 1];
      }
      break;

    case TENSOR_LAYOUT_COLUMN_MAJOR:
      // Column-major: leftmost dimension changes fasters
      tensor->strides[0] = 1;
      for (size_t i = 1; i < tensor->ndim; i++)
      {
        tensor->strides[i] = tensor->strides[i - 1] * tensor->shape[i - 1];
      }
      break;

    case TENSOR_LAYOUT_NCHW:
      // N, C, H, W layout (common in many frameworks)
      if (tensor->ndim != 4)
      {
        // Fall back to row-maor for non 4D-tensors
        tensor->layout = TENSOR_LAYOUT_ROW_MAJOR;
        calculate_strides(tensor); // Recursive call with row-major
        return;
      }

      // W, H, C, N order of strides
      tensor->strides[3] = 1; // W
      tensor->strides[2] = tensor->shape[3]; // H
      tensor->strides[1] = tensor->strides[2] * tensor->shape[2]; // C
      tensor->strides[0] = tensor->strides[1] * tensor->shape[1]; // N
      break;

    case TENSOR_LAYOUT_NHWC:
      // N, H, W, C layout (Tensorflow default)
      if (tensor->ndim != 4)
      {
        // Fall back to row-major for non-4D tensors
        tensor->layout = TENSOR_LAYOUT_ROW_MAJOR;
        calculate_strides(tensor); // Recursive call with row-major
        return;
      }

      // C, W, H, N order of strides
      tensor->strides[3] = 1; // C
      tensor->strides[2] = tensor->shape[3]; // W
      tensor->strides[1] = tensor->strides[2] * tensor->shape[2]; // H
      tensor->strides[0] = tensor->strides[1] * tensor->shape[1]; // N
      break;
  }
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
    // No suitable block found, try to allocate from system
    size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    void* new_block = kpage_alloc_contiguous(pages, PAGEF_ZEROED);

    if (!new_block)
    {
      // Out of memory
      return NULL;
    }

    // Align the new block
    void* aligned_block = align_pointer(new_block, alignment);
    size_t alignment_overhead = (uintptr_t)aligned_block - (uintptr_t)new_block;

    // Check if we need to create a free block for the alignment overhead
    if (alignment_overhead > 0)
    {
      // TODO: Add this space to free blocks
    }

    // Update statistics
    ai_mem_pool.used_size += pages * PAGE_SIZE;
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

  // Calculate block size (must be tracked elsewhere in a real implementation)
  // For simplicity, we assume it's not a small block and we'll free it to the paging system

  // Add to free list if there's space
  if (ai_mem_pool.free_block_count < AI_MEMORY_MAX_BLOCKS)
  {
    // TODO: determine real block size
    size_t block_size = PAGE_SIZE; // Assume one page for now

    ai_mem_pool.free_blocks[ai_mem_pool.free_block_count] = ptr;
    ai_mem_pool.free_block_sizes[ai_mem_pool.free_block_count] = block_size;
    ai_mem_pool.free_block_count++;

    // Update statistics
    ai_mem_pool.used_size -= block_size;
    ai_mem_pool.deallocations++;

    return EOK;
  }

  // Free list is full, return directly to system
  // This is simplified, real implementation would determine the actual page count
  kpage_free(ptr);

  // Update statistics
  ai_mem_pool.used_size -= PAGE_SIZE; // Assume one page
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
  uart_send_string(" KB pool...\n");

  // Clean pool structure
  memset(&ai_mem_pool, 0, sizeof(ai_memory_pool_t));
  ai_mem_pool.total_size = requested_pool_size;

  // Allocate management structures
  ai_mem_pool.free_blocks = (void**)kmalloc(AI_MEMORY_MAX_BLOCKS * sizeof(void*));
  if (!ai_mem_pool.free_blocks) {
    uart_send_string("Failed to allocate free block array\n");
    return -ENOMEM;
  }

  ai_mem_pool.free_block_sizes = (size_t*)kmalloc(AI_MEMORY_MAX_BLOCKS * sizeof(size_t));
  if (!ai_mem_pool.free_block_sizes) {
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
  if (!ai_mem_pool.small_block_bitmap) {
    kfree(ai_mem_pool.free_block_sizes);
    kfree(ai_mem_pool.free_blocks);
    uart_send_string("Failed to allocate small block bitmap\n");
    return -ENOMEM;
  }

  // Clear bitmap
  memset(ai_mem_pool.small_block_bitmap, 0, bitmap_size);
  
  // Calculate number of pages needed
  size_t pages_needed = small_pool_size / PAGE_SIZE;
  uart_send_string("Allocating ");
  uart_send_string(uint_to_str(pages_needed));
  uart_send_string(" pages for small block pool...\n");
  
  // Try contiguous allocation first
  void* memory = kpage_alloc_contiguous(pages_needed, PAGEF_ZEROED);
  
  if (!memory) {
    // Fall back to allocating individual pages
    uart_send_string("Contiguous allocation failed, trying individual pages...\n");
    
    // Allocate pages individually and track them in free blocks
    size_t pages_allocated = 0;
    
    for (size_t i = 0; i < pages_needed && i < AI_MEMORY_MAX_BLOCKS; i++) {
      void* page = kpage_alloc_flags(PAGEF_ZEROED);
      if (page) {
        // Add to free blocks list
        ai_mem_pool.free_blocks[ai_mem_pool.free_block_count] = page;
        ai_mem_pool.free_block_sizes[ai_mem_pool.free_block_count] = PAGE_SIZE;
        ai_mem_pool.free_block_count++;
        pages_allocated++;
        
        if (pages_allocated % 32 == 0 || pages_allocated == pages_needed) {
          uart_send_string("Allocated ");
          uart_send_string(uint_to_str(pages_allocated));
          uart_send_string(" / ");
          uart_send_string(uint_to_str(pages_needed));
          uart_send_string(" pages\n");
        }
      } else {
        uart_send_string("WARNING: Page allocation stopped at ");
        uart_send_string(uint_to_str(pages_allocated));
        uart_send_string(" pages\n");
        break;
      }
    }
    
    if (pages_allocated == 0) {
      // Fall back to minimal allocation (one page)
      uart_send_string("Failed to allocate multiple pages, falling back to single page...\n");
      void* page = kpage_alloc_flags(PAGEF_ZEROED);
      
      if (!page) {
        uart_send_string("Critical failure: Cannot allocate even a single page\n");
        kfree(ai_mem_pool.small_block_bitmap);
        kfree(ai_mem_pool.free_block_sizes);
        kfree(ai_mem_pool.free_blocks);
        return -ENOMEM;
      }
      
      // Set up minimal structures
      ai_mem_pool.small_block_pool = page;
      ai_mem_pool.small_block_count = PAGE_SIZE / AI_MEMORY_MIN_BLOCK_SIZE;
      ai_mem_pool.total_size = PAGE_SIZE;
    } else {
      // Use the pages we managed to allocate
      uart_send_string("Using ");
      uart_send_string(uint_to_str(pages_allocated));
      uart_send_string(" non-contiguous pages for AI memory\n");
      
      // Use the first allocated page as the small block pool
      ai_mem_pool.small_block_pool = ai_mem_pool.free_blocks[0];
      
      // Remove it from the free list
      for (size_t i = 0; i < ai_mem_pool.free_block_count - 1; i++) {
        ai_mem_pool.free_blocks[i] = ai_mem_pool.free_blocks[i + 1];
        ai_mem_pool.free_block_sizes[i] = ai_mem_pool.free_block_sizes[i + 1];
      }
      ai_mem_pool.free_block_count--;
      
      // Adjust the small block count for the actual allocation
      ai_mem_pool.small_block_count = PAGE_SIZE / AI_MEMORY_MIN_BLOCK_SIZE;
      ai_mem_pool.total_size = pages_allocated * PAGE_SIZE;
    }
  } else {
    // Successfully allocated contiguous memory
    uart_send_string("Successfully allocated contiguous memory block at: 0x");
    uart_send_string(uint_to_str((uintptr_t)memory));
    uart_send_string("\n");
    
    ai_mem_pool.small_block_pool = memory;
    ai_mem_pool.total_size = pages_needed * PAGE_SIZE;
    
    // Add the memory to the free blocks list as a single large block
    ai_mem_pool.free_blocks[0] = memory;
    ai_mem_pool.free_block_sizes[0] = pages_needed * PAGE_SIZE;
    ai_mem_pool.free_block_count = 1;
  }
  
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
  if (!shape || ndim == 0 || dtype >= TENSOR_TYPE_COUNT)
  {
    return NULL;
  }

  // Calculate total elements and memory size
  size_t total_elems = 1;
  for (size_t i = 0; i < ndim; i++)
  {
    total_elems *= shape[i];
  }

  size_t elem_size = ai_tensor_get_elem_size(dtype);
  size_t memory_size = total_elems * elem_size;

  // Determine alignment
  size_t alignment = 8; // Default alignment
  if (flags & TENSOR_MEM_ALIGNED)
  {
    alignment = ai_memory_get_optimal_alignment(dtype);
  }

  // Allocate tensor descriptor
  tensor_t* tensor = (tensor_t*)kmalloc(sizeof(tensor_t));
  if (!tensor)
  {
    return NULL;
  }

  // Clearn tensor
  memset(tensor, 0, sizeof(tensor_t));

  // Allocate shape array
  tensor->shape = (size_t*)kmalloc(ndim * sizeof(size_t));
  if (!tensor->shape)
  {
    kfree(tensor);
    return NULL;
  }

  // Copy shape
  for (size_t i = 0; i < ndim; i++)
  {
    tensor->shape[i] = shape[i];
  }

  // Allocate strides array
  tensor->strides = (size_t*)kmalloc(ndim * sizeof(size_t));
  if (!tensor->strides)
  {
    kfree(tensor->shape);
    kfree(tensor);
    return NULL;
  }

  // Set tensor properties
  tensor->ndim = ndim;
  tensor->dtype = dtype;
  tensor->elem_size = elem_size;
  tensor->layout = layout;
  tensor->flags = flags;

  // Calculate strides
  calculate_strides(tensor);

  // Allocate tensor data
  tensor->data = ai_memory_alloc(memory_size, alignment);
  if (!tensor->data)
  {
    kfree(tensor->strides);
    kfree(tensor->shape);
    kfree(tensor);
    return NULL;
  }

  // Zero data if requested
  if (flags & TENSOR_MEM_ZEROED)
  {
    memset(tensor->data, 0, memory_size);
  }

  return tensor;
}

/**
 * @brief Destroy a tensor and free its memory
 * 
 * @param tensor Pointer to the tensor to destroy
 * @return int Status code (EOK on success, -EINVARG on invalid argument)
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
int ai_tensor_destroy(tensor_t* tensor)
{
  if (!tensor)
  {
    return -EINVARG;
  }

  // Free tensor data
  if (tensor->data)
  {
    ai_memory_free(tensor->data);
  }

  // Free shape and strides array
  if (tensor->shape)
  {
    kfree(tensor->shape);
  }

  if (tensor->strides)
  {
    kfree(tensor->strides);
  }

  // Free tensor descriptor
  kfree(tensor);

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

  // Copy chape and strides
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
