# Core subsystem Makefile
# Author: Fedi Nabli
# Date: 26 Feb 2025
# Last Modified: 17 Mar 2025

# Directories definitions
BUILD_DIR := ../build/core
INCLUDES := -I../arch/arm64/includes -I../includes

# Source files
OBJ_FILES := $(BUILD_DIR)/kernel_main.o $(BUILD_DIR)/mmu/mmu.o $(BUILD_DIR)/memory/memory.o $(BUILD_DIR)/memory/heap/heap.o $(BUILD_DIR)/memory/heap/kheap.o $(BUILD_DIR)/memory/paging/paging.o

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

# Compile mmu file
$(BUILD_DIR)/mmu/mmu.o: mmu/mmu.c | $(BUILD_DIR)/mmu
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/mmu/mmu.o mmu/mmu.c

# Compile memory file
$(BUILD_DIR)/memory/memory.o: memory/memory.c | $(BUILD_DIR)/memory
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/memory/memory.o memory/memory.c

# Compile memory heap file
$(BUILD_DIR)/memory/heap/heap.o: memory/heap/heap.c | $(BUILD_DIR)/memory/heap
	$(CC) $(CFLAGS) -I./memory/heap -c -o $(BUILD_DIR)/memory/heap/heap.o memory/heap/heap.c

# Compile memory kernel heap file
$(BUILD_DIR)/memory/heap/kheap.o: memory/heap/kheap.c | $(BUILD_DIR)/memory/heap
	$(CC) $(CFLAGS) -I./memory/heap -I../includes/synapse/memory/heap -I../includes/synapse/memory/heap -c -o $(BUILD_DIR)/memory/heap/kheap.o memory/heap/kheap.c

# Compile memory paging file
$(BUILD_DIR)/memory/paging/paging.o: memory/paging/paging.c | $(BUILD_DIR)/memory/paging
	$(CC) $(CFLAGS) -I../includes/synapse/memory/paging -c -o $(BUILD_DIR)/memory/paging/paging.o memory/paging/paging.c

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean