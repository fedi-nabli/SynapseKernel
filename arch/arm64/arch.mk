# ARM64 Architecture Configuration
# Author: Fedi Nabli
# Date: 26 Feb 2025
# Last Modified: 26 Feb 2025

# Target Architecture
ARCH := arm64
CPU := cortex-a53

# Assembly flags for aarch64-linux-gnu
ASFLAGS := -mcpu=$(CPU) -g

# Architecture specific compiler flags
# For bare-metal development with aarch64-linux-gnu toolchain
ARCH_CFLAGS := -mcpu=$(CPU) -mgeneral-regs-only -mno-outline-atomics

# Boot info magic number
BOOT_INFO_MAGIC := 0x424F4F54
CFLAGS += -DBOOT_INFO_MAGIC=$(BOOT_INFO_MAGIC)

# Add architecture specific includes
ARCH_INCLUDES := -I$(CURDIR)/arch/$(ARCH)/includes

# Export architecture flags
CFLAGS += $(AACH_CFLAGS) $(ARCH_INCLUDES)