#ifndef __SYNAPSE_MEMORY_AI_MEMORY_H_
#define __SYNAPSE_MEMORY_AI_MEMORY_H_

#include <synapse/bool.h>
#include <synapse/types.h>

// Tensor data types
typedef enum
{
  TENSOR_TYPE_INT8,     // 8-bit integer
  TENSOR_TYPE_INT16,    // 16-bit integer
  TENSOR_TYPE_INT32,    // 32-bit integer
  TENSOR_TYPE_FLOAT16,  // 16-bit floating point
  TENSOR_TYPE_FLOAT32,  // 32-bit floating point
  TENSOR_TYPE_COUNT     // Number of tensor types
} tensor_dtype_t;

// Tensor memory layout
typedef enum
{
  TENSOR_LAYOUT_ROW_MAJOR,      // Row-major layout (default)
  TENSOR_LAYOUT_COLUMN_MAJOR,   // Column major layout
  TENSOR_LAYOUT_NCHW,           // Batch, Channels, Height, Width (for CNN)
  TENSOR_LAYOUT_NHWC            // Batch, Height, Width, Channels (Tensorflow default)
} tensor_layout_t;

// Tensor memory flags
typedef enum
{
  TENSOR_MEM_ZEROED     = (1 << 0), // Zero the memory
  TENSOR_MEM_ALIGNED    = (1 << 1), // Use optimal alignment
  TENSOR_MEM_CONTIGUOUS = (1 << 2), // Ensure contiguous allocation
  TENSOR_MEM_CACHEABLE  = (1 << 3), // Allow caching (default)
  TENSOR_MEM_UNCACHEABLE = (1 << 4), // Bypass cache
  TENSOR_MEM_DMA        = (1 << 5) // DMA-friendly memory
} tensor_mem_flags_t;

// Tensor descriptor
typedef struct
{
  void* data; // Pointer to tensor data
  size_t* shape; // Array of dimensions
  size_t* strides; // Strides for each dimension
  size_t ndim; // Number of dimensions
  size_t elem_size; // Size of each element in bytes
  tensor_dtype_t dtype; // Data type
  tensor_layout_t layout; // Memory layour
  uint32_t flags; // Memory flags
} tensor_t;

/**
 * @brief Initialize the AI memory subsystem
 * 
 * @param pool_size Size of the memory pool to allocate
 * @return int Status code (EOK on success, -ENOMEM on allocation failure)
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
int ai_memory_init(size_t pool_size);

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
tensor_t* ai_tensor_create(size_t* shape, size_t ndim, tensor_dtype_t dtype, tensor_layout_t layout, uint32_t flags);

/**
 * @brief Destroy a tensor and free its memory
 * 
 * @param tensor Pointer to the tensor to destroy
 * @return int Status code (EOK on success, -EINVARG on invalid argument)
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
int ai_tensor_destroy(tensor_t* tensor);

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
int ai_tensor_reshape(tensor_t* tensor, size_t* new_shape, size_t new_ndim);

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
int ai_tensor_set_layout(tensor_t* tensor, tensor_layout_t new_layout);

/**
 * @brief Get tensor memory alignment
 * 
 * @param tensor Pointer to the tensor
 * @return size_t Alignment size in bytes
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
size_t ai_tensor_get_alignment(tensor_t* tensor);

/**
 * @brief Get total tensor size in bytes
 * 
 * @param tensor Pointer to the tensor
 * @return size_t Total size of the tensor in bytes
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
size_t ai_tensor_get_size(tensor_t* tensor);

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
void* ai_tensor_get_element(tensor_t* tensor, size_t* indices);

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
int ai_tensor_copy_data(tensor_t* tensor, void* data, size_t size);

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
tensor_t* ai_tensor_view(tensor_t* tensor, size_t* start_indices, size_t* shape);

/**
 * @brief Print AI memory pool statistics
 * 
 * @author Fedi Nabli
 * @date 21 Mar 2025
 */
void ai_memory_print_stats();

#endif