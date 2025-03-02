# ARM64 Architecture Makefile
# Author: Fedi Nabli
# Date: 26 Feb 2025
# Last Modified: 26 Feb 2025

BUILD_DIR := ../../build/arch

# Subdirectories
BOOT_DIR := boot
INTERRUPT_DIR := interrupt
UART_DIR := uart

# Object files
BOOT_OBJS := $(BUILD_DIR)/$(BOOT_DIR)/start.o $(BUILD_DIR)/$(BOOT_DIR)/boot.o
INTERRUPT_OBJS := $(BUILD_DIR)/$(INTERRUPT_DIR)/vector.o
UART_OBJS := $(BUILD_DIR)/$(UART_DIR)/uart.o

# All object files
ALL_OBJS := $(BOOT_OBJS) $(INTERRUPT_OBJS) $(UART_OBJS)

# Default target
all: $(ALL_OBJS)

# Ensure build directories
$(BUILD_DIR)/$(BOOT_DIR) $(BUILD_DIR)/$(INTERRUPT_DIR) $(BUILD_DIR)/$(UART_DIR):
	@mkdir -p $@

# Compile bootloader stage 1 assembly files
$(BUILD_DIR)/$(BOOT_DIR)/start.o: $(BOOT_DIR)/start.S | $(BUILD_DIR)/$(BOOT_DIR)
	$(AS) $(ASFLAGS) -o $(BUILD_DIR)/$(BOOT_DIR)/start.o $(BOOT_DIR)/start.S

# Compile bootloader stage 2 assembly files
$(BUILD_DIR)/$(BOOT_DIR)/boot.o: $(BOOT_DIR)/boot.S | $(BUILD_DIR)/$(BOOT_DIR)
	$(AS) $(ASFLAGS) -o $(BUILD_DIR)/$(BOOT_DIR)/boot.o $(BOOT_DIR)/boot.S

# Compile interrupt assembly file
$(BUILD_DIR)/$(INTERRUPT_DIR)/vector.o: $(INTERRUPT_DIR)/vector.S | $(BUILD_DIR)/$(INTERRUPT_DIR)
	$(AS) $(ASFLAGS) -o $(BUILD_DIR)/$(INTERRUPT_DIR)/vector.o $(INTERRUPT_DIR)/vector.S

# Compile uart assembly file
$(BUILD_DIR)/$(UART_DIR)/uart.o: $(UART_DIR)/uart.S | $(BUILD_DIR)/$(UART_DIR)
	$(AS) $(ASFLAGS) -o $(BUILD_DIR)/$(UART_DIR)/uart.o $(UART_DIR)/uart.S

print_order:
	@echo "Build Order:"
	@echo "		Boot: $(BOOT_OBJS)"
	@echo "		Interrupt: $(INTERRUPT_OBJS)"
	@echo "		UART: $(UART_OBJS)"

# Clean build artifacts
clean:
	rm -rf $(ALL_OBJS)

.PHONY: all clean
