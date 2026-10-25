# core — the kernel's starting point

```
modules/core/
└── kernel_main.c   brings up every part of the OS, then runs the shell
```

## Where it fits

```
BIOS -> bootloader -> boot/boot.s (_start) -> kernel_main()  <- you are here
```

`boot/boot.s` sets up a stack and calls `kernel_main(magic, mb_info)`.
From there, `kernel_main()` reads like a table of contents. Each step only
uses the steps before it:

| # | Step                 | What it sets up                          | Code lives in |
|---|----------------------|------------------------------------------|---------------|
| 0 | `vga_init`, banner   | font, theme, 80x25 console, so every step can report | `device_management/vga`, `ui/core/logo` |
| 1 | `setup_cpu`          | GDT + TSS, IDT, PIC remap                | `arch/x86`, `device_management/pic` |
| 2 | `setup_devices`      | timer (100 Hz), keyboard, mouse, clock   | `device_management` |
| 3 | `setup_memory`       | PMM (frames), VMM (paging), heap         | `memory_management` |
| 4 | `setup_security`     | attack/defence pairs; SHA-256 of the kernel's code | `security_management`, each module's `security/` |
| 5 | `setup_shell`        | shell + every module's commands          | `shell`, each module's `commands/` |
| 6 | `setup_filesystem`   | RAMDisk with the root `/`                | `filesystem_management` |
| 7 | `setup_processes`    | scheduler; kernel_main becomes a process | `process_management` |
| 8 | `setup_syscalls`     | `int 0x80` handler                       | `process_management/syscalls` |
| 9 | `sti`                | hardware interrupts on                   | |
| 10 | `statusbar_start`   | the top bar and its daemon, statusd      | `ui/core/statusbar` |

After that, `kernel_main()` never returns. It loops forever: take a key
from the keyboard buffer and give it to the shell, or `hlt` until the
next interrupt.

## The boot log

Every step prints one line, like Linux does:

```
[ .... ] Physical memory              <- before the step runs
[  OK  ] Physical memory           127 MB RAM, 30340 frames free
[ SKIP ] PS/2 mouse                no mouse found: the shell works without it
[FAILED] Physical memory           not started by a multiboot bootloader
```

The details are real numbers read back from each part, so the log is
also a quick check that every layer came up the way you expect.

## Why this order?

- **Screen first**, so every later step can print its result.
- **CPU tables before devices**: an IRQ with no IDT entry crashes the CPU.
- **Memory before processes and files**: they need `kmalloc`.
- **`sti` late**: once interrupts are on, the timer starts switching
  tasks, so everything must be ready.
- **The status bar last**: its daemon is a process, so it needs the
  scheduler, and it shows the clock, memory and processes.

If a step fails and nothing after it can work (for example, no memory
map from the bootloader), `boot_fail()` prints `[FAILED]` with the
reason and halts.
