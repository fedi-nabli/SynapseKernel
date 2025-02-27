# Core subsystem Makefile
# Author: Fedi Nabli
# Date: 26 Feb 2025

# Directories definitions
BUILD_DIR := ../build/core
INCLUDES := -I../arch/arm64/includes

# Source files
C_SRCS := $(wildcard *.c)
OBJ_FILES := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS))

# Compile flags
CFLAGS += $(INCLUDES) -Wall -Wextra

# Default target
all: $(OBJ_FILES)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile C files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean