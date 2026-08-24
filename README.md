# MyOS

MyOS is a simple educational operating system developed from scratch to learn about low-level systems engineering.

## Current version

v0.1

## Current features

- x86_64 kernel
- Bootable in QEMU (using GRUB/Multiboot2)
- Kernel entry point
- Basic terminal output (VGA Text Mode)
- Safe CPU halt loop
- Make-based build system
- Basic GDB debugging support

## Build

To compile the operating system and generate the bootable ISO file (`build/myos.iso`), run:

```bash
make
```
*(Note: requires `gcc`, `nasm`, `make`, `xorriso`, and `mtools`)*

## Run

To start the operating system inside the QEMU emulator, run:

```bash
make run
```

## Debug

To debug the operating system, you can start QEMU in a paused state, waiting for a GDB connection:

```bash
make debug
```

Then, in a second terminal window, launch GDB and connect to QEMU:

```bash
gdb
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```
*(You can use `layout src` and `info registers` in GDB to inspect the state).*

## Architecture

MyOS follows a minimalistic architecture to transition from the bootloader to a high-level C environment.

```text
Bootloader (GRUB)
    ↓
Kernel (boot.S: 32-bit to 64-bit transition)
    ↓
Kernel (boot64.S: 64-bit entry point)
    ↓
Kernel (kernel.c: C environment)
    ↓
Terminal (terminal.c: VGA Output)
    ↓
CPU halt (boot64.S: Safe infinite loop)
```

## Educational Documentation

### 1. The Boot Process
1. **Power on / VM starts**: QEMU begins execution, running its virtual firmware (SeaBIOS).
2. **Bootloader**: The firmware finds the GRUB bootloader on our ISO image and executes it. GRUB reads `grub.cfg`, finds our Multiboot2 kernel, and loads it into memory at physical address `1MB`.
3. **Kernel loaded**: GRUB transfers control to the `start` label in `boot.S`. At this point, the CPU is in 32-bit Protected Mode.
4. **Kernel entry point**: `boot.S` sets up 4-level paging (identity mapping the first 2MB), enables PAE, enables Long Mode in the EFER MSR, and enables Paging. It loads a 64-bit GDT and does a far jump to `long_mode_start` in `boot64.S`.
5. **kernel_main()**: `boot64.S` sets up the C stack pointer and calls `kernel_main` in `kernel.c`.
6. **Terminal initialization**: `kernel_main` calls `terminal_initialize()`, which clears the VGA text buffer located at physical address `0xB8000`.
7. **Print message**: The kernel writes characters and color bytes directly to `0xB8000`. The video card hardware automatically renders this memory to the screen.
8. **CPU halt loop**: After `kernel_main` returns, `boot64.S` enters an infinite loop using the `hlt` instruction, putting the CPU in a low-power state until an interrupt occurs (which we have disabled via `cli`).

### 2. Files Created

- **`Makefile`**: Automates the build process. Compiles C and assembly files, links them into an ELF binary, creates the GRUB config, and packages everything into an ISO.
- **`linker.ld`**: Tells the linker how to arrange the sections (like `.text` and `.data`) in the final binary, ensuring the Multiboot header comes first and the kernel is loaded at `1MB`.
- **`boot.S`**: The 32-bit assembly entry point. Contains the Multiboot2 header and the crucial code to transition the CPU from 32-bit to 64-bit mode by setting up page tables and the GDT.
- **`boot64.S`**: The 64-bit assembly entry point. Sets up the stack for C code, calls `kernel_main`, and handles the final infinite halt loop.
- **`terminal.c` / `terminal.h`**: A basic VGA text mode driver. Provides an abstraction to print strings to the screen without cluttering the main kernel logic.
- **`kernel.c`**: The main C entry point. Orchestrates the initialization and prints the welcome message.
