# MyOS

MyOS is a simple educational operating system developed from scratch to learn about low-level systems engineering.

## Current version

v0.1

## Current features

- x86_64 kernel
- Bootable in QEMU (Multiboot)
- Kernel entry point
- Basic terminal output (VGA Text Mode)
- Safe CPU halt loop
- Make-based build system
- Basic GDB debugging support

## Build

To compile the operating system, run:

```bash
make
```
*(Note: requires `gcc`, `nasm`, `make`, `ld`, and `objcopy`)*

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
QEMU (Multiboot Loader)
    ↓
Kernel (boot.asm: 32-bit to 64-bit transition)
    ↓
Kernel (boot64.asm: 64-bit entry point)
    ↓
Kernel (kernel.c: C environment)
    ↓
Terminal (terminal.c: VGA Output)
    ↓
CPU halt (boot64.asm: Safe infinite loop)
```

## Educational Documentation

### 1. The Boot Process
1. **Power on / VM starts**: QEMU begins execution with Multiboot support (`-kernel build/myos.bin`).
2. **Bootloader**: QEMU's Multiboot loader loads our kernel into memory at physical address `1MB`.
3. **Kernel entry point**: Control is transferred to the `start` label in `boot.asm`. At this point, the CPU is in 32-bit Protected Mode.
4. **Transition to Long Mode**: `boot.asm` sets up 4-level paging (identity mapping the first 2MB), enables PAE, enables Long Mode in the EFER MSR, and enables Paging. It loads a 64-bit GDT and does a far jump to `long_mode_start` in `boot64.asm`.
5. **kernel_main()**: `boot64.asm` sets up the C stack pointer and calls `kernel_main` in `kernel.c`.
6. **Terminal initialization**: `kernel_main` calls `terminal_initialize()`, which clears the VGA text buffer located at physical address `0xB8000`.
7. **Print message**: The kernel writes characters and color bytes directly to `0xB8000`. The video card hardware automatically renders this memory to the screen.
8. **CPU halt loop**: After `kernel_main` returns, `boot64.asm` enters an infinite loop using the `hlt` instruction, putting the CPU in a low-power state until an interrupt occurs (which we have disabled via `cli`).

### 2. Files Created

- **`Makefile`**: Automates the build process. Compiles C and NASM assembly files, links them into an ELF binary, and generates the runnable image.
- **`linker.ld`**: Tells the linker how to arrange the sections (like `.text` and `.data`) in the final binary, ensuring the Multiboot header comes first and the kernel is loaded at `1MB`.
- **`boot/boot.asm`**: The 32-bit NASM assembly entry point. Contains the Multiboot header and the code to transition the CPU from 32-bit to 64-bit mode by setting up page tables and the GDT.
- **`boot/boot64.asm`**: The 64-bit NASM assembly entry point. Sets up the stack for C code, calls `kernel_main`, and handles the final infinite halt loop.
- **`kernel/terminal.c` / `include/terminal.h`**: A basic VGA text mode driver. Provides an abstraction to print strings to the screen without cluttering the main kernel logic.
- **`kernel/kernel.c`**: The main C entry point. Orchestrates the initialization and prints the welcome message.
