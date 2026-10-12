# Core Module

## Educational Concepts: Kernel Entry & Initialization
This module contains the fundamental entry points for the kernel after the bootloader has handed over control. 
When a computer boots, the BIOS/UEFI loads the bootloader (like GRUB), which then loads the kernel into memory based on the Multiboot specification.
The bootloader passes control to the `_start` assembly routine, which sets up a basic C stack and calls the `kernel_main` function in C.

Here, we also interact directly with hardware via Memory Mapped I/O (MMIO). The VGA text buffer is mapped to memory address `0xB8000`. By writing specific 16-bit values (8 bits for the character, 8 bits for the foreground and background colors) to this memory region, we can display text on the screen without needing complex graphics drivers.
