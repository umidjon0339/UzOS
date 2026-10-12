# UzOS Learning Guide

Welcome to **UzOS**, a 32-bit x86 educational operating system! This guide is designed to help you understand the entire working structure of the OS from scratch. If you want to learn how an operating system works, or how to contribute to UzOS, this is the perfect place to start.

---

## 1. The Boot Process: Where it all Begins

When the computer turns on, the BIOS hands control over to a bootloader (like GRUB). The bootloader loads the UzOS kernel into memory and jumps to our entry point.

### `boot/boot.s`
This is the very first piece of code that runs. It's written in Assembly because we need to set up the most basic environment before C code can run.
* **What it does:** Defines the Multiboot header (so GRUB recognizes it), sets up a tiny stack (16KB), and calls `kernel_main`.

### `modules/core/core/kernel_main.c`
This is the heart of the initialization phase. If you want to see how the OS is put together, look at `kernel_main()`. It boots up every subsystem in a very specific order:
1. **VGA Driver**: Initializes text-mode video so we can print to the screen.
2. **GDT (Global Descriptor Table)**: Sets up memory segments (Ring 0 Kernel vs Ring 3 User).
3. **IDT (Interrupt Descriptor Table)**: Sets up the handlers for CPU exceptions and hardware interrupts.
4. **PIC & Timers/Keyboard**: Remaps hardware interrupts and turns on the timer and keyboard.
5. **Memory Management**: 
   * **PMM** (Physical Memory Manager): Tracks free RAM pages.
   * **VMM** (Virtual Memory Manager): Sets up paging.
   * **KHEAP**: Sets up `kmalloc` for dynamic memory allocation.
6. **Shell**: Starts the CLI and registers commands.
7. **Filesystem**: Initializes the Virtual File System (VFS) and the initial RAMDisk.
8. **Process Management**: Turns on the scheduler to allow multitasking.
9. **System Calls**: Registers the `int 0x80` handler for user-space to kernel-space communication.
10. **Enable Interrupts (`sti`)**: Finally, allows the CPU to receive hardware interrupts, entering the idle loop.

---

## 2. Directory Structure Overview

UzOS is structured modularly. Here is what every folder does:

* `arch/x86/`: Contains architecture-specific code. If we ever ported UzOS to ARM, this is the only folder that would drastically change. It manages the GDT, IDT, and Interrupt Service Routines (ISRs).
* `boot/`: The initial assembly boot sequence.
* `include/`: All the shared `.h` header files. If a module needs to expose a function, its header goes here.
* `userland/`: Contains user-space programs (like `hello.c`). These are compiled into `.elf` executables that the OS can load and run.
* `modules/`: **The core of the OS.** Every major feature is isolated into its own module folder.

---

## 3. The `modules/` Architecture

To make the OS highly educational, every major subsystem in `modules/` is heavily structured and isolated. 

Let's look at the standard architecture of a complex module (like `memory_management`, `filesystem_management`, or `process_management`):

```text
modules/filesystem_management/
├── core/
│   └── vfs.c          (The core logic of the module)
├── commands/
│   ├── ls/cmd_ls.c    (A specific CLI command)
│   ├── cat/cmd_cat.c
│   └── register/register.c (Registers all commands to the shell)
├── system_calls/
│   ├── open/sys_open.c (A specific system call exposed to Userland)
│   ├── read/sys_read.c
│   └── handler/syscall_handler.c
└── security/
    └── check_path/fs_security_check_path.c (Security validations)
```

### Breakdown of Module Folders:
1. **`core/`**: The pure logic of the subsystem. For memory, this is the PMM and VMM. For processes, this is the Scheduler and ELF loader.
2. **`commands/`**: User-facing shell commands. Each command lives in its own folder and file (e.g., `cmd_kill.c`). This makes it incredibly easy to add new commands without cluttering a single massive file.
3. **`system_calls/`**: Interfaces for user-space applications. Like commands, each syscall (e.g., `sys_read`, `sys_mmap`) gets its own file.
4. **`security/`**: Validation checks. Before a system call does anything dangerous, it routes through these security files to ensure pointers are valid and permissions are met.

---

## 4. Deep Dive into Major Modules

### A. Device Drivers (`modules/device_drivers/`)
Contains code to talk to specific hardware.
* `vga_display`: Text mode printing.
* `keyboard` & `mouse`: PS/2 input drivers.
* `timer`: The PIT (Programmable Interval Timer) that fires an interrupt 100 times a second to drive the scheduler.
* `pic`: Programmable Interrupt Controller.
* `gui`: A basic graphical user interface using BGA (Bochs Graphics Adapter) and a Window Manager.

### B. Memory Management (`modules/memory_management/`)
* **PMM (`pmm.c`)**: Manages physical RAM. It uses a bitmap to track which 4KB frames of RAM are free or used.
* **VMM (`vmm.c`)**: Manages Paging. It maps Virtual Addresses to Physical Addresses so processes can have isolated memory spaces.
* **KHEAP (`kheap.c`)**: Implements `kmalloc()` and `kfree()` so the kernel can allocate memory dynamically.

### C. Process Management (`modules/process_management/`)
* **Process (`process.c`)**: Manages the Process Control Block (PCB) state (PID, registers, memory space).
* **Scheduler (`scheduler.c`)**: Uses Round-Robin scheduling. Every time the hardware timer fires, the scheduler saves the state of the current process and loads the state of the next ready process (Context Switching).
* **ELF Loader (`elf.c`)**: Reads an Executable and Linkable Format file from the filesystem, maps it into memory, and creates a process for it.

### D. Filesystem Management (`modules/filesystem_management/`)
* **VFS (`vfs.c`)**: An abstraction layer. It defines what a "file" or "directory" is, regardless of the underlying disk.
* **RAMDisk (`ramdisk.c`)**: An in-memory filesystem. Since we don't have hard drive drivers yet, files are created and stored directly in RAM.

---

## 5. How to Contribute / Extend

### Adding a new Shell Command (Command Package Pattern)
Each command in UzOS is designed as a self-contained package inside its own directory (e.g., `modules/process_management/commands/ps/`):
1. **Create the folder**: `modules/<subsystem>/commands/<mycmd>/`
2. **`cmd_<mycmd>.h`**: Define command metadata (`CMD_NAME`, `CMD_DESCRIPTION`), prototype, and `cmd_<mycmd>_register()`.
3. **`cmd_<mycmd>.c`**: Implement the command logic and self-registration function.
4. **`README.md`**: Add educational documentation explaining the theory, data structures, and how to test the command.
5. **Register**: Call `cmd_<mycmd>_register()` inside the subsystem's `register/register.c`.

### Adding a new System Call
1. Define the system call number and prototype in `include/` (e.g., `proc_syscalls.h`).
2. Create a new folder and `.c` file in the module's `system_calls/` directory (e.g., `my_syscall/sys_my_syscall.c`).
3. Implement the logic.
4. Route the syscall number to your function in the generic `syscall_handler` (located in `modules/process_management/system_calls/handler/syscall_handler.c`).

Enjoy exploring UzOS! Start by modifying `kernel_main.c` or adding a new command to see your code come to life.
