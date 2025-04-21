/*
 * process.h - Process management structures and funcitons
 *
 * Author: Fedi Nabli
 * Date: 2 Apr 2025
 * Last Modified: 9 Apr 2025
 */

#ifndef __SYNAPSE_PROCESS_PROCESS_H_
#define __SYNAPSE_PROCESS_PROCESS_H_

#include <kernel/config.h>

#include <synapse/bool.h>
#include <synapse/types.h>
#include <synapse/task/task.h>

// Process memory allocations structure
struct process_allocation
{
  void* ptr;
  size_t size;
};

struct process_arguments
{
  int argc;
  char** argv;
};

// Process structure
struct process
{
  // Identity
  pid_t id; // Process ID
  char name[SYNAPSE_MAX_PROCESS_NAME]; // Process name

  struct task* task; // Main execution task

  // Process memory
  struct process_allocation allocations[SYNAPSE_MAX_PROCESSES_ALLOCATIONS];

  union
  {
    void* ptr; // Binary data pointer
    // TODO: add elf file
  };
  uint64_t size; // Size of code segment
  void* stack; // Base pointer to stack memory

  struct process_arguments arguments;
};

/**
 * Process allocation
 */
/**
 * @brief Allocate memory for a process
 * 
 * @param process Process to allocate for
 * @param size Size in bytes to allocatr
 * @return void* Pointer to allocated memory, NULL on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
void* process_malloc(struct process* process, size_t size);

/**
 * @brief Free memory allocated to a process
 * 
 * @param process Process that owns the memory
 * @param ptr Pointer to memory to free
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_free(struct process* process, void* ptr);

/**
 * @brief Get total memory allocated to a process
 * 
 * @param process Process to check
 * @return size_t Total bytes allocated
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
size_t process_get_memory_usage(struct process* process);

/**
 * @brief Verify if memory address belongs to a process
 * 
 * This function is particularly important for AArch64 without MMU/paging
 * as we need to validate memory access from user space
 * 
 * @param process Process to check
 * @param addr Memory address to verify
 * @param size Size of memory range to check
 * @return true if the memory range belongs to the process
 * @return false if the memory range doesn't belong to the process
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
bool process_memory_verify(struct process* process, void* addr, size_t size);

/**
 * @brief Verify if memory address belongs to a process
 * 
 * This function is particularly important for AArch64 without MMU/paging
 * as we need to validate memory access from user space
 * 
 * @param process Process to check
 * @param addr Memory address to verify
 * @param size Size of memory range to check
 * @return true if the memory range belongs to the process
 * @return false if the memory range doesn't belong to the process
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
bool process_memory_verify(struct process* process, void* addr, size_t size);

/**
 * @brief Flush instruction cache for memory region
 * 
 * This is important for AArch64 when modifying code in memory
 * 
 * @param addr Start address
 * @param size Size of region
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_memory_flush_icache(void* addr, size_t size);

/**
 * @brief Get the currently running process
 * 
 * @return struct process* Pointer to current process, NULL if none is running
 * 
 * @author Fedi Nabli
 * @date 2 Apr 2025
 */
struct process* process_current();

/**
 * @brief Get a process by ID
 * 
 * @param id Process ID to look up
 * @return struct process* Pointer to process, NULL if not found
 * 
 * @author Fedi Nabli
 * @date 2 Apr 2025
 */
struct process* process_get(pid_t id);

/**
 * @brief Create and load a process from memory
 * 
 * @param name Process name
 * @param program_data Pointer to program binary
 * @param size Size of program binary
 * @param process_out Output parameter for created process
 * @return int Process ID on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_create(const char* name, void* program_data, size_t size, struct process** process_out);

/**
 * @brief Create a process in a specific slot
 * 
 * @param name Process name
 * @param program_data Pointer to program binary
 * @param size Size of program binary
 * @param slot Process slot/ID to use
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_create_for_slot(const char* name, void* program_data, size_t size, int slot);

/**
 * @brief Create a process and immediatly switch to it
 * 
 * @param name Process name
 * @param program_data Pointer to program binary
 * @param size Size of program binary
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_create_switch(const char* name, void* program_data, size_t size);

/**
 * @brief Switch to different process by ID
 * 
 * @param id Process ID to switch to
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_switch(pid_t id);

/**
 * @brief Terminate process and free resources
 * 
 * @param id Process ID to terminate
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 7 Apr 2025
 */
int process_terminate(pid_t id);

/**
 * @brief Get process arguments
 * 
 * @param id Process ID
 * @param argc Output for argument count
 * @param argv Output for argument vector
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int process_get_arguments(pid_t id, int* argc, char*** argv);

/**
 * @brief Set process arguments
 * 
 * @param process Process to set arguments for
 * @param argc Argument count
 * @param argv Argument vector
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 8 Apr 2025
 */
int process_set_arguments(struct process* process, int argc, char** argv);


/// PROCESS MANAGEMENT FUNCTIONS ///
/**
 * @brief Initialize process management subsystem
 * 
 * @return int EOK on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int process_management_init();

/**
 * @brief Create and start initial kernel process
 * 
 * @param entry_point Function pointer to process entry point
 * @param name Process name
 * @return int Process ID on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int create_kernel_process(void (*entry_point)(void), const char* name);

/**
 * @brief Create and start initial user process
 * 
 * @param entry_point Function pointer to process entry point
 * @param name Process name
 * @return int Process ID on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int create_user_process(void (*entry_point)(void), const char* name);

/**
 * @brief Start the process management subsystem and run initial task
 * 
 * @return int Never returns on success, negative error code on failure
 * 
 * @author Fedi Nabli
 * @date 9 Apr 2025
 */
int process_management_start();

#endif