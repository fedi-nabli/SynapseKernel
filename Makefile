# Main Kernel Makefile
# Author: Fedi Nabli
# Date: 26 Feb 2025

# Toolchain definitions for aarch64-linux-gnu
CROSS_COMPILE ?= aarch64-linux-gnu-
CC := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
AS := $(CROSS_COMPILE)as
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
AR := $(CROSS_COMPILE)ar
STRIP := $(CROSS_COMPILE)strip
# Standard GDB (doesn't use the cross-compile prefix)
GDB := gdb

# Build directories
BUILD_DIR := build
ARCH_DIR := arch/arm64
ARCH_BUILD_DIR := $(BUILD_DIR)/arch
CORE_BUILD_DIR := $(BUILD_DIR)/core

# Target binary image
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

# Architecture-specific flags
include $(ARCH_DIR)/arch.mk

# Global compiler flags
# -ffreestanding and -nostdlib are crucial for bare-metal development
CFLAGS += -mcpu=cortex-a53 -Wall -Wextra -ffreestanding -nostdlib -nostartfiles \
          -fno-builtin -fno-stack-protector -fno-common -O2 -g

# Linker flags and script
LDFLAGS := -nostdlib -T $(ARCH_DIR)/linker.ld -Map=$(BUILD_DIR)/kernel.map

# Export variables to sub-makefiles
export CC CXX AS LD OBJCOPY OBJDUMP AR STRIP GDB
export CFLAGS CXXFLAGS ASFLAGS LDFLAGS

# Default target
all: directories kernel

# Create build directories
directories:
	@mkdir -p $(BUILD_DIR) $(ARCH_BUILD_DIR)/boot $(ARCH_BUILD_DIR)/interrupt $(ARCH_BUILD_DIR)/uart $(CORE_BUILD_DIR)

# Build subsystems
arch:
	$(MAKE) -C $(ARCH_DIR)

core:
	$(MAKE) -C core

# Link kernel
kernel: arch core
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) \
		$(ARCH_BUILD_DIR)/boot/boot.o \
		$(ARCH_BUILD_DIR)/boot/start.o \
		$(ARCH_BUILD_DIR)/interrupt/vector.o \
		$(ARCH_BUILD_DIR)/uart/uart.o \
		$(CORE_BUILD_DIR)/kernel_main.o
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)
	$(OBJDUMP) -D $(KERNEL_ELF) > $(BUILD_DIR)/kernel.dump

# Clean build artifacts
clean:
	$(MAKE) -C $(ARCH_DIR) clean
	$(MAKE) -C core clean
	rm -rf $(BUILD_DIR)

# Run in QEMU (if applicable)
run: all
	qemu-system-aarch64 -M virt -cpu cortex-a53 -m 1G -nographic -kernel $(KERNEL_BIN)

# Debug with GDB
debug: all
	@echo "Starting QEMU with GDB server on port 1234..."
	@echo "In another terminal, run: gdb -ex 'target remote localhost:1234' -ex 'file $(KERNEL_ELF)'"
	qemu-system-aarch64 -M virt -cpu cortex-a53 -m 1G -nographic -kernel $(KERNEL_BIN) -S -s

.PHONY: all clean directories arch core kernel run debug
