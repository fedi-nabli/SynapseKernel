# Main Kernel Makefile
# Author: Fedi Nabli
# Date: 26 Feb 2025
# Last Modified: 31 Mar 2025

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

# Final Binary directory
BIN_DIR := bin

# Target binary image
KERNEL_ELF := $(BIN_DIR)/kernel.elf
KERNEL_BIN := $(BIN_DIR)/kernel.bin

# Architecture-specific flags
include $(ARCH_DIR)/arch.mk

# Global compiler flags
# -ffreestanding and -nostdlib are crucial for bare-metal development
CFLAGS += -mcpu=cortex-a53 -Wall -Wextra -ffreestanding -nostdlib -nostartfiles \
          -fno-builtin -fno-stack-protector -fno-common -O2 -g \
					-Wno-unused-parameter -Wno-unused-label -Werror \
					-finline-functions -fomit-frame-pointer \
					-falign-jumps -falign-functions -falign-labels -falign-loops

# Linker flags and script
LDFLAGS := -nostdlib -T $(ARCH_DIR)/linker.ld -Map=$(BUILD_DIR)/kernel.map

# Export variables to sub-makefiles
export CC CXX AS LD OBJCOPY OBJDUMP AR STRIP GDB
export CFLAGS CXXFLAGS ASFLAGS LDFLAGS

# Default target
all: directories kernel

# Create build directories
directories:
	@mkdir -p $(BIN_DIR) $(BUILD_DIR) $(ARCH_BUILD_DIR)/boot $(ARCH_BUILD_DIR)/interrupt $(ARCH_BUILD_DIR)/uart $(CORE_BUILD_DIR)  $(CORE_BUILD_DIR)/memory $(CORE_BUILD_DIR)/memory/heap $(CORE_BUILD_DIR)/memory/ai_memory $(CORE_BUILD_DIR)/string $(CORE_BUILD_DIR)/interrupts $(CORE_BUILD_DIR)/timer $(CORE_BUILD_DIR)/task $(CORE_BUILD_DIR)/process $(CORE_BUILD_DIR)/scheduler

# Build subsystems
arch:
	$(MAKE) -C $(ARCH_DIR)

core:
	$(MAKE) -C core

# Link kernel
kernel: arch core
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) \
		$(ARCH_BUILD_DIR)/boot/start.o \
		$(ARCH_BUILD_DIR)/boot/boot.o \
		$(ARCH_BUILD_DIR)/interrupt/vector.o \
		$(ARCH_BUILD_DIR)/uart/uart.o \
		$(CORE_BUILD_DIR)/memory/memory.o \
		$(CORE_BUILD_DIR)/memory/heap/heap.o \
		$(CORE_BUILD_DIR)/memory/heap/kheap.o \
		$(CORE_BUILD_DIR)/memory/ai_memory/ai_memory.o \
		$(CORE_BUILD_DIR)/memory/memory_system.o \
		$(CORE_BUILD_DIR)/string/string.o \
		$(CORE_BUILD_DIR)/interrupts/interrupt.o \
		$(CORE_BUILD_DIR)/interrupts/svc.o \
		$(CORE_BUILD_DIR)/interrupts/syscall.o \
		$(CORE_BUILD_DIR)/timer/timer.o \
		$(CORE_BUILD_DIR)/task/context_switch.o \
		$(CORE_BUILD_DIR)/task/task.o \
		$(CORE_BUILD_DIR)/process/process.o \
		$(CORE_BUILD_DIR)/process/process_memory.o \
		$(CORE_BUILD_DIR)/scheduler/scheduler.o \
		$(CORE_BUILD_DIR)/process/process_management_init.o \
		$(CORE_BUILD_DIR)/kernel_main.o
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)
	$(OBJDUMP) -D $(KERNEL_ELF) > $(BUILD_DIR)/kernel.dump
	@echo "Kernel build successfully:"
	@echo "		ELF: $(KERNEL_ELF)"
	@echo "		BIN: $(KERNEL_BIN)"

# Clean build artifacts
clean:
	$(MAKE) -C $(ARCH_DIR) clean
	$(MAKE) -C core clean
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)

# Run in QEMU (if applicable)
run: all
	qemu-system-aarch64 -M virt,gic-version=2 -cpu cortex-a53 -m 1G -nographic -kernel $(KERNEL_BIN) -smp 1

# Debug with GDB
debug: all
	@echo "Starting QEMU with GDB server on port 1234..."
	@echo "In another terminal, run: gdb -ex 'target remote localhost:1234' -ex 'file $(KERNEL_ELF)'"
	qemu-system-aarch64 -M virt,gic-version=2 -cpu cortex-a53 -m 1G -smp 1 -nographic -kernel $(KERNEL_BIN) -S -s

.PHONY: all clean directories arch core kernel run debug
