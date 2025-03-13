# ARM64 Architecture Makefile
# Author: Fedi Nabli
# Date: 26 Feb 2025
# Last Modified: 13 Mar 2025

BUILD_DIR := ../../build/arch

# Subdirectories
BOOT_DIR := boot
INTERRUPT_DIR := interrupt
UART_DIR := uart
MMU_DIR := mmu

# Object files
BOOT_OBJS := $(BUILD_DIR)/$(BOOT_DIR)/start.o $(BUILD_DIR)/$(BOOT_DIR)/boot.o
INTERRUPT_OBJS := $(BUILD_DIR)/$(INTERRUPT_DIR)/vector.o
UART_OBJS := $(BUILD_DIR)/$(UART_DIR)/uart.o
MMU_OBJS := $(BUILD_DIR)/$(MMU_DIR)/mmu.o $(BUILD_DIR)/$(MMU_DIR)/arm_mmu.o

INCLUDES := -I./includes -I../../includes

# Compile flags
CFLAGS += $(INCLUDES) -Wall -Wextra

# All object files
ALL_OBJS := $(BOOT_OBJS) $(INTERRUPT_OBJS) $(UART_OBJS) $(MMU_OBJS)

# Default target
all: $(ALL_OBJS)

# Ensure build directories
$(BUILD_DIR)/$(BOOT_DIR) $(BUILD_DIR)/$(INTERRUPT_DIR) $(BUILD_DIR)/$(UART_DIR) $(BUILD_DIR)/$(MMU_DIR):
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

# Compile mmu assembly file
$(BUILD_DIR)/$(MMU_DIR)/mmu.o: $(MMU_DIR)/mmu.S | $(BUILD_DIR)/$(MMU_DIR)
	$(AS) $(ASFLAGS) -o $(BUILD_DIR)/$(MMU_DIR)/mmu.o $(MMU_DIR)/mmu.S

# Compile mmu c file
$(BUILD_DIR)/$(MMU_DIR)/arm_mmu.o: $(MMU_DIR)/arm_mmu.c | $(BUILD_DIR)/$(MMU_DIR)
	$(CC) $(CFLAGS) -c -o $(BUILD_DIR)/$(MMU_DIR)/arm_mmu.o mmu/arm_mmu.c

print_order:
	@echo "Build Order:"
	@echo "		Boot: $(BOOT_OBJS)"
	@echo "		Interrupt: $(INTERRUPT_OBJS)"
	@echo "		UART: $(UART_OBJS)"

# Clean build artifacts
clean:
	rm -rf $(ALL_OBJS)

.PHONY: all clean
