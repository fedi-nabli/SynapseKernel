# Core subsystem Makefile
# Author: Fedi Nabli
# Date: 26 Feb 2025

# Directories definitions
BUILD_DIR := ../build/core
INCLUDES := -I../arch/arm64/includes

# Source files
OBJ_FILES := $(BUILD_DIR)/kernel_main.o

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

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean