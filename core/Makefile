# Core subsystem Makefile
# Author: Fedi Nabli
# Date: 26 Feb 2025
# Last Modified: 31 Mar 2025

# Directories definitions
BUILD_DIR := ../build/core
INCLUDES := -I../arch/arm64/includes -I../includes

# Source files
OBJ_FILES := $(BUILD_DIR)/kernel_main.o $(BUILD_DIR)/memory/memory.o $(BUILD_DIR)/memory/heap/heap.o $(BUILD_DIR)/memory/heap/kheap.o $(BUILD_DIR)/memory/ai_memory/ai_memory.o $(BUILD_DIR)/memory/memory_system.o $(BUILD_DIR)/string/string.o $(BUILD_DIR)/interrupts/interrupt.o $(BUILD_DIR)/task/context_switch.o $(BUILD_DIR)/interrupts/svc.o $(BUILD_DIR)/interrupts/syscall.o $(BUILD_DIR)/timer/timer.o $(BUILD_DIR)/task/task.o $(BUILD_DIR)/process/process.o $(BUILD_DIR)/process/process_memory.o $(BUILD_DIR)/scheduler/scheduler.o $(BUILD_DIR)/process/process_management_init.o

# Compile flags
CFLAGS += $(INCLUDES) -Wall -Wextra

# Default target
all: $(OBJ_FILES)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile C kernel main file
$(BUILD_DIR)/kernel_main.o: kernel_main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/kernel_main.o kernel_main.c

# Compile memory file
$(BUILD_DIR)/memory/memory.o: memory/memory.c | $(BUILD_DIR)/memory
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/memory/memory.o memory/memory.c

# Compile memory heap file
$(BUILD_DIR)/memory/heap/heap.o: memory/heap/heap.c | $(BUILD_DIR)/memory/heap
	$(CC) $(CFLAGS) -I./memory/heap -c -o $(BUILD_DIR)/memory/heap/heap.o memory/heap/heap.c

# Compile memory kernel heap file
$(BUILD_DIR)/memory/heap/kheap.o: memory/heap/kheap.c | $(BUILD_DIR)/memory/heap
	$(CC) $(CFLAGS) -I./memory/heap -I../includes/synapse/memory/heap -I../includes/synapse/memory/heap -c -o $(BUILD_DIR)/memory/heap/kheap.o memory/heap/kheap.c

# Compile memory ai memory file
$(BUILD_DIR)/memory/ai_memory/ai_memory.o: memory/ai_memory/ai_memory.c | $(BUILD_DIR)/memory/ai_memory
	$(CC) $(CFLAGS) -I../includes/synapse/memory/ai_memory -c -o $(BUILD_DIR)/memory/ai_memory/ai_memory.o memory/ai_memory/ai_memory.c

# Compile memory system file
$(BUILD_DIR)/memory/memory_system.o: memory/memory_system.c | $(BUILD_DIR)/memory
	$(CC) $(CFLAGS) -I../includes/synapse/memory -c -o $(BUILD_DIR)/memory/memory_system.o memory/memory_system.c

# Compile string file
$(BUILD_DIR)/string/string.o: string/string.c | $(BUILD_DIR)/string
	$(CC) $(CFLAGS) -I../includes/synapse/string -c -o $(BUILD_DIR)/string/string.o string/string.c

# Compile interrupt file
$(BUILD_DIR)/interrupts/interrupt.o: interrupts/interrupt.c | $(BUILD_DIR)/interrupts
	$(CC) $(CFLAGS) -I../includes/synapse/interrupts -c -o $(BUILD_DIR)/interrupts/interrupt.o interrupts/interrupt.c

# Compile svc file
$(BUILD_DIR)/interrupts/svc.o: interrupts/svc.c | $(BUILD_DIR)/interrupts
	$(CC) $(CFLAGS) -I../includes/synapse/interrupts -c -o $(BUILD_DIR)/interrupts/svc.o interrupts/svc.c

# Compile syscall file
$(BUILD_DIR)/interrupts/syscall.o: interrupts/syscall.c | $(BUILD_DIR)/interrupts
	$(CC) $(CFLAGS) -I../includes/synapse/interrupts -c -o $(BUILD_DIR)/interrupts/syscall.o interrupts/syscall.c

# Compile context switching assembly files
$(BUILD_DIR)/task/context_switch.o: task/context_switch.S | $(BUILD_DIR)/task
	$(AS) $(ASFLAGS) -o $(BUILD_DIR)/task/context_switch.o task/context_switch.S

# Compile task file
$(BUILD_DIR)/timer/timer.o: timer/timer.c | $(BUILD_DIR)/timer
	$(CC) $(CFLAGS) -I../includes/synapse/timer -c -o $(BUILD_DIR)/timer/timer.o timer/timer.c

# Compile task file
$(BUILD_DIR)/task/task.o: task/task.c | $(BUILD_DIR)/task
	$(CC) $(CFLAGS) -I../includes/synapse/task -c -o $(BUILD_DIR)/task/task.o task/task.c

# Compile process file
$(BUILD_DIR)/process/process.o: process/process.c | $(BUILD_DIR)/process
	$(CC) $(CFLAGS) -I../includes/synapse/process -c -o $(BUILD_DIR)/process/process.o process/process.c

# Compile process memory file
$(BUILD_DIR)/process/process_memory.o: process/process_memory.c | $(BUILD_DIR)/process
	$(CC) $(CFLAGS) -I../includes/synapse/process -c -o $(BUILD_DIR)/process/process_memory.o process/process_memory.c

# Compile scheduler file
$(BUILD_DIR)/scheduler/scheduler.o: scheduler/scheduler.c | $(BUILD_DIR)/scheduler
	$(CC) $(CFLAGS) -I../includes/synapse/scheduler -c -o $(BUILD_DIR)/scheduler/scheduler.o scheduler/scheduler.c

# Compile process management file
$(BUILD_DIR)/process/process_management_init.o: process/process_management_init.c | $(BUILD_DIR)/process
	$(CC) $(CFLAGS) -I../includes/synapse/process -c -o $(BUILD_DIR)/process/process_management_init.o process/process_management_init.c

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean