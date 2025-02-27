# ARM64 Architecture Makefile
# Author: Fedi Nabli
# Date: 26 Feb 2025

BUILD_DIR := ../../build/arch

# Subdirectories
BOOT_DIR := boot
INTERRUPT_DIR := interrupt
UART_DIR := uart

# Source files
BOOT_SRCS := $(wildcard $(BOOT_DIR)/*.S)
INTERRUPT_SRCS := $(wildcard $(INTERRUPT_DIR)/*.S)
UART_SRCS := $(wildcard $(UART_DIR)/*.S)

# Object files
BOOT_OBJS := $(patsubst $(BOOT_DIR)/%.S,$(BUILD_DIR)/$(BOOT_DIR)/%.o,$(BOOT_SRCS))
INTERRUPT_OBJS := $(patsubst $(INTERRUPT_DIR)/%.S,$(BUILD_DIR)/$(INTERRUPT_DIR)/%.o,$(INTERRUPT_SRCS))
UART_OBJS := $(patsubst $(UART_DIR)/%.S,$(BUILD_DIR)/$(UART_DIR)/%.o,$(UART_SRCS))

# All object files
ALL_OBJS := $(BOOT_OBJS) $(INTERRUPT_OBJS) $(UART_OBJS)

# Default target
all: $(ALL_OBJS)

# Ensure build directories
$(BUILD_DIR)/$(BOOT_DIR) $(BUILD_DIR)/$(INTERRUPT_DIR) $(BUILD_DIR)/$(UART_DIR):
	@mkdir -p $@

# Compile boot assembly files
$(BUILD_DIR)/$(BOOT_DIR)/%.o: $(BOOT_DIR)/%.S | $(BUILD_DIR)/$(BOOT_DIR)
	$(AS) $(ASFLAGS) -o $@ $<

# Compile interrupt assembly file
$(BUILD_DIR)/$(INTERRUPT_DIR)/%.o: $(INTERRUPT_DIR)/%.S | $(BUILD_DIR)/$(INTERRUPT_DIR)
	$(AS) $(ASFLAGS) -o $@ $<

# Compile uart assembly file
$(BUILD_DIR)/$(UART_DIR)/%.o: $(UART_DIR)/%.S | $(BUILD_DIR)/$(UART_DIR)
	$(AS) $(ASFLAGS) -o $@ $<

# Clean build artifacts
clean:
	rm -rf $(ALL_OBJS)

.PHONY: all clean
