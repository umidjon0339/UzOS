# boot — from bootloader to `kernel_main()`

```
boot/
├── boot.s        multiboot header, boot stack, _start entry point
├── multiboot.h   struct the bootloader passes us (memory map, ...)
└── linker.ld     where each section of kernel.bin goes in memory
```

## What happens at power on

1. The BIOS loads a bootloader (GRUB, or QEMU's built-in one with `-kernel`).
2. The bootloader looks for the **multiboot header** in the first 8 KiB of
   `kernel.bin`, loads the kernel at **1 MiB** and switches to 32-bit
   protected mode.
3. It jumps to **`_start`** (`boot.s`) with:
   - `EAX` = `0x2BADB002` (proof that a multiboot loader started us)
   - `EBX` = address of a `multiboot_info_t` (see `multiboot.h`)
4. `_start` clears interrupts and the direction flag, sets `ESP` to a
   16 KiB stack, and calls `kernel_main(magic, info)`.

## Memory layout (`linker.ld`)

```
0x00100000  .text     code (multiboot header first)
            .rodata   constants, strings
            .data     initialised globals
            .bss      zeroed globals + boot stack
            _kernel_end  (page aligned)
```

Each section starts on a 4 KiB page. The PMM uses `_kernel_start` and
`_kernel_end` to mark the kernel's frames as used.

Check the header with `make check`, which uses `grub-file`.
