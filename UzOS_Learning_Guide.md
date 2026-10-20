# UzOS Learning Guide

Welcome to **UzOS**, a 32-bit x86 educational operating system! This guide is designed to help you understand the entire working structure of the OS from scratch. If you want to learn how an operating system works, or how to contribute to UzOS, this is the perfect place to start.

---

## 1. The Boot Process: Where it all Begins

When the computer turns on, the BIOS hands control over to a bootloader (like GRUB). The bootloader loads the UzOS kernel into memory and jumps to our entry point.

### `boot/boot.s`
This is the very first piece of code that runs. It's written in Assembly because we need to set up the most basic environment before C code can run.
* **What it does:** Defines the Multiboot header (so GRUB recognizes it), sets up a tiny stack (16KB), and calls `kernel_main`.
* See [boot/README.md](boot/README.md) for the full boot flow and memory layout.

### `modules/core/kernel_main.c`
This is the heart of the initialization phase. If you want to see how the OS is put together, look at `kernel_main()`. It boots up every subsystem in a very specific order, and prints one Linux-style line per step (`[  OK  ] Physical memory   127 MB RAM, ...`):
1. **Console**: the font, the colour theme and 80x25 text, so every step can report. Then the UzOS banner.
2. **CPU**: asks the CPU who it is and what it can do (`cpuid`).
3. **GDT (Global Descriptor Table)**: Sets up memory segments (Ring 0 Kernel vs Ring 3 User).
4. **IDT (Interrupt Descriptor Table)**: Sets up the handlers for CPU exceptions and hardware interrupts. Then **CPU exceptions**: every fault gets a handler, so a fault in a user program kills only that program.
5. **PIC & devices**: Remaps hardware interrupts; starts the timer, keyboard, mouse, and reads the real-time clock.
6. **Memory Management**: 
   * **PMM** (Physical Memory Manager): Tracks free RAM pages.
   * **VMM** (Virtual Memory Manager): Sets up paging.
   * **KHEAP**: Sets up `kmalloc` for dynamic memory allocation.
7. **Security lab**: registers every module's attack/defence pairs.
8. **Shell**: Starts the CLI and registers commands.
9. **Filesystem**: Initializes the Virtual File System (VFS) and the initial RAMDisk.
10. **Process Management**: Turns on the scheduler to allow multitasking.
11. **System Calls**: Registers the `int 0x80` handler for user-space to kernel-space communication.
12. **Enable Interrupts (`sti`)**: allows the CPU to receive hardware interrupts.
13. **CPU clock**: counts CPU cycles over 10 timer ticks to learn the clock speed, then makes `rdtsc` kernel-only.
14. **Status bar**: starts `statusd`, a daemon that keeps the top row up to date. Then the shell takes over.

---

## 2. Directory Structure Overview

UzOS is structured modularly. Here is what every folder does:

* `arch/x86/`: Contains architecture-specific code. If we ever ported UzOS to ARM, this is the only folder that would drastically change. One folder per feature (`io/`, `cpu/`, `cpuid/`, `gdt/`, `tss/`, `idt/`, `isr/`, `usermode/`), each holding its own `.h`, `.c` and `.s`. See [arch/x86/README.md](arch/x86/README.md).
* `boot/`: The initial assembly boot sequence, the multiboot info struct, and the linker script.
* `lib/`: A tiny C library for the kernel (`memset`, `strcmp`, `format` - a small snprintf, ...), since a kernel has no standard library. See [lib/README.md](lib/README.md).
* `userland/`: Contains user-space programs (like `hello.c`). These are compiled into `.elf` executables that the OS can load and run.
* `modules/`: **The core of the OS.** Every major feature is isolated into its own module folder.

---

## 3. The `modules/` Architecture

To make the OS highly educational, every major subsystem in `modules/` is heavily structured and isolated. 

Let's look at the standard architecture of a complex module (like `memory_management`, `filesystem_management`, or `process_management`):

```text
modules/<module>/
├── core/<part>/          the machinery, one folder per part (e.g. pmm/, vmm/)
├── commands/<name>/      one shell command per folder + one command table
├── syscalls/<name>/      one system call per folder: its number, .h and .c
└── security/
    ├── attack/<id>/      how a guarantee breaks (+ README)
    └── defence/<id>/     the check the kernel runs (+ README)
```

### Breakdown of Module Folders:
1. **`core/`**: The pure logic of the subsystem. For memory, this is the PMM and VMM. For processes, this is the scheduler and ELF loader.
2. **`commands/`**: User-facing shell commands, one folder each (e.g. `commands/kill/cmd_kill.c`), registered from one small table.
3. **`syscalls/`**: The door for user programs. Each syscall has a folder with its number (`SYS_READ`), header and code; `process_management/syscalls/handler/` routes `int 0x80` to all of them.
4. **`security/`**: Paired attacks and defences. Every file starts with what it is, what it uses and why; every folder has a README. The `seclab` command (in `modules/security_lab/`) runs them.

Every module now follows this layout and has a README with its full map.

---

## 4. Deep Dive into Major Modules

### A. Device Management (`modules/device_management/`)
See [modules/device_management/README.md](modules/device_management/README.md) for the full map. One folder per driver:
* `pic/`: the interrupt controller. All IRQ lines start masked; each driver opens its own.
* `timer/`: the PIT at 100 Hz. Every tick is charged to a process (cpu_management) and runs the scheduler.
* `keyboard/` & `mouse/`: PS/2 input. The mouse decoder resynchronises if a byte is lost; the wheel scrolls terminal history.
* `rtc/`: the real-time clock chip: the date and time (`date`, the `time` syscall).
* `vga/`: the console (text, colours, scroll-back, the status row). It draws through a **display** interface: `vga/text/` (80x25, 80x50) or `gui/fbcon/` (1024x768 graphics, 128x48 characters on a wallpaper). `vga/palette/` holds the colour themes.
* `font/`: the UzOS font (based on Terminus) and UzOS's own icons (folder, check, crescent...).
* `gui/`: video card (`bga`), drawing (`canvas`), double buffering (`screen`) and windows (`wm`).
* **Commands**: `uptime`, `date`, `devices` (IRQ lines and interrupt counts), `startx` (ESC returns to the shell), `beep`, `play`, and for the screen `theme`, `font`, `mode`.

### B. I/O Management (`modules/io_management/`)
See [modules/io_management/README.md](modules/io_management/README.md).
* **File descriptors (`core/fd/`)**: small numbers that stand for open files. 0, 1 and 2 are the keyboard and the screen; `open` hands out 3 and up.
* **Syscalls**: `open` (with `O_READ`, `O_WRITE`, `O_CREATE`, `O_TRUNC`, `O_APPEND`), `read`, `write`, `close`, `seek`.
* **Commands**: `ioinfo [pid]` shows a process's descriptors.

### C. Memory Management (`modules/memory_management/`)
See [modules/memory_management/README.md](modules/memory_management/README.md) for the full map.
* **Layout (`core/layout/memory_layout.h`)**: One picture of the whole address space: NULL page, identity-mapped kernel, heap, user space, framebuffer.
* **PMM (`core/pmm/`)**: Physical RAM as a bitmap of 4 KB frames. Firmware areas, the kernel image and boot data are reserved and can never be freed.
* **VMM (`core/vmm/`)**: Paging. Maps virtual pages to frames through the page directory and page tables (reached through a "recursive" directory entry). A page fault in a user program kills only that program.
* **KHEAP (`core/kheap/`)**: `kmalloc()` / `kfree()` with a checked block list.
* **User memory (`core/umem/`, `syscalls/`)**: `sbrk`, `mmap` and `munmap` give programs zeroed pages in user space. Try `memtest` and `pagewalk <addr>`.

### D. Process Management (`modules/process_management/`)
See [modules/process_management/README.md](modules/process_management/README.md) for the full map.
* **Process (`core/process/`)**: The Process Control Block (`process_t`) table: create, kill, exit. Finished processes become zombies and the reaper frees them.
* **Scheduler (`core/scheduler/`)**: The mechanism of multitasking. When a turn ends, `switch_task.s` saves the registers and EFLAGS of the current process on its stack and resumes the next one (context switching). *Which* process is next is decided by cpu_management.
* **User tasks (`core/usertask/`)**: copies a small assembly program to a user page and drops it to ring 3. `testuser` and several security lab attacks use it.
* **ELF Loader (`core/elf/`)**: Reads an ELF file from the filesystem, checks it, maps it into memory, and creates a process for it.
* **System calls (`syscalls/`)**: `int 0x80` with the number in EAX. `syscalls/handler/` routes it to `exit`, `getpid`, `sleep` or `yield`.
* **Security lab (`modules/security_lab/`)**: The shared registry and the `seclab` command. Each module keeps its own attack/defence pairs.

### E. CPU Management (`modules/cpu_management/`)
See [modules/cpu_management/README.md](modules/cpu_management/README.md) for the full map.
* **Info (`core/info/`)**: decodes `cpuid`: vendor, family/model/stepping, and feature flags (`tsc`, `pae`, `sse`, ...). The kernel asks `cpu_has(CPU_FEAT_TSC)` before it uses a feature.
* **Faults (`core/faults/`)**: a handler for every CPU exception. A fault in ring 3 (divide by zero, `cli`, `ud2`, ...) kills that process with a `[CPU FAULT]` report; a fault in ring 0 is a kernel bug and halts.
* **TSC (`core/tsc/`)**: the cycle counter, and the clock speed measured against the PIT.
* **Accounting (`core/accounting/`)**: every timer tick is charged to the process it interrupted, in ring 3 or ring 0, or to idle: CPU time per process and the load of the last second.
* **Priority (`core/priority/`)**: the scheduling policy. Levels 1–7 (7 first, 0 = the idle task), 50 ms turns, equal levels take turns, and **aging** so a busy high process cannot starve the others.
* **Syscalls**: `cputime`, `setpriority`, `getpriority`.
* **Commands**: `cpuinfo [flags]`, `faults [list]`, `cpustat`, `nice [pid [level]]`. `ps` and `top` show PRI and CPU%.

### F. Filesystem Management (`modules/filesystem_management/`)
See [modules/filesystem_management/README.md](modules/filesystem_management/README.md).
* **VFS (`core/vfs/`)**: the tree of files and directories, path lookup (`/a/b`, `../x`), and open counts.
* **RAMDisk (`core/ramdisk/`)**: the driver that keeps every file's bytes in the kernel heap.
* **Commands**: `ls cd pwd mkdir touch rm cat write cp mv wc grep hexdump`, all with full paths.

### G. Shell (`modules/shell/`)
See [modules/shell/README.md](modules/shell/README.md).
* **Core**: the command registry, the line being typed (`line/`), history (Up/Down) and Tab completion for commands and paths.
* **Commands**: `help` (built from the registry), `clear`, `echo`, `color`, `calc` (a recursive-descent parser), `history`, `snake`.
* **Prompt**: `root@uzos:/docs$`, like a Linux shell.

### H. UI (`modules/ui/`)
See [modules/ui/README.md](modules/ui/README.md).
* **Style (`core/style/`)**: one look for every message: `ui_error`, `ui_ok`, `ui_warn`, `ui_usage`; colours by meaning, not by number.
* **Frames and bars (`core/box/`)**: `┌─ Title ─┐` panels, table cells that cannot be broken by a strange name, `███░░` bars. `meminfo`, `heapinfo`, `ps`, `devices` and `ioinfo` use them.
* **Boot log, logo, status bar**: the `[  OK  ]` lines, the banner and flag, and the top bar kept fresh by the `statusd` daemon.
* **Commands**: `uzfetch` (a system summary next to the flag), `statusbar [on|off]`.

---

## 5. How to Contribute / Extend

### Adding a new Shell Command
Every module follows the same pattern:
1. **Create the folder**: `modules/<module>/commands/<mycmd>/cmd_<mycmd>.c`, starting with a short comment: what it does, what it uses, why.
2. **Declare it** in the module's `commands/<module>_commands.h`.
3. **Register it**: add one line to the command table in `commands/<module>_commands.c`.

### Adding a new System Call
1. **Create the folder**: `modules/<module>/syscalls/<name>/` with `sys_<name>.h` (the `SYS_<NAME>` number and the prototype) and `sys_<name>.c`.
2. **Route it**: include `sys_<name>.h` in `modules/process_management/syscalls/handler/syscall_handler.c`, write a small `do_<name>()` wrapper (check pointer arguments with `buffer_ok` / `path_ok`), and add one line to the table.
3. Arguments come in EBX, ECX, EDX; the wrapper's return value goes back to the program in EAX.

### Printing from a command
Use the ui module, not raw colours: `ui_error("mycmd", "what went wrong")`, `ui_usage("mycmd <arg>")`, `ui_ok(...)`. For a table, `box_begin` / `box_row` / `ui_cell` / `box_end`.

Enjoy exploring UzOS! Start by modifying `kernel_main.c` or adding a new command to see your code come to life.
