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
7. **Security**: registers every module's attack/defence pairs, then **measures the kernel**: the SHA-256 of its code and constants, to compare with later.
8. **Shell**: Starts the CLI and registers commands.
9. **Filesystem**: Initializes the Virtual File System (VFS) and the initial RAMDisk. Then **Users**: root and guest, each with a home directory under `/home`.
10. **Process Management**: Turns on the scheduler to allow multitasking.
11. **System Calls**: Registers the `int 0x80` handler for user-space to kernel-space communication.
12. **Enable Interrupts (`sti`)**: allows the CPU to receive hardware interrupts.
13. **CPU clock**: counts CPU cycles over 10 timer ticks to learn the clock speed, then makes `rdtsc` kernel-only.
14. **Status bar**: starts `statusd`, a daemon that keeps the top row up to date.
15. **Login**: `uzos login:` asks for a name and password (root / uzos or guest / guest). Then the shell takes over, running as that user.

---

## 2. Directory Structure Overview

UzOS is structured modularly. Here is what every folder does:

* `arch/x86/`: Contains architecture-specific code. If we ever ported UzOS to ARM, this is the only folder that would drastically change. One folder per feature (`io/`, `cpu/`, `cpuid/`, `gdt/`, `tss/`, `idt/`, `isr/`, `usermode/`), each holding its own `.h`, `.c` and `.s`. See [arch/x86/README.md](arch/x86/README.md).
* `boot/`: The initial assembly boot sequence, the multiboot info struct, and the linker script.
* `lib/`: A tiny C library for the kernel (`memset`, `strcmp`, `format` - a small snprintf, ...), since a kernel has no standard library. See [lib/README.md](lib/README.md).
* `userland/`: Contains user-space programs (like `hello.c`). These are compiled into `.elf` executables that the OS can load and run.
* `modules/`: **The core of the OS.** Every major feature is isolated into its own module folder: device, io, memory, process, cpu, user, security, filesystem and network management, the shell and the ui.

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
4. **`security/`**: Paired attacks and defences. Every file starts with what it is, what it uses and why; every folder has a README. The `seclab` command (in `modules/security_management/`) runs them.

Every module now follows this layout and has a README with its full map.

---

## 4. Deep Dive into Major Modules

### A. Device Management (`modules/device_management/`)
See [modules/device_management/README.md](modules/device_management/README.md) for the full map. One folder per driver:
* `pic/`: the interrupt controller. All IRQ lines start masked; each driver opens its own.
* `timer/`: the PIT at 100 Hz. Every tick is charged to a process (cpu_management) and runs the scheduler.
* `keyboard/` & `mouse/`: PS/2 input. The mouse decoder resynchronises if a byte is lost; the wheel scrolls terminal history.
* `rtc/`: the real-time clock chip: the date and time (`date`, the `time` syscall).
* `vga/`: the console (text, colours, scroll-back, the status row). It draws through a **display** interface: `vga/text/` (80x25, 80x50) or `gui/fbcon/` (graphics from 800x600 up to 1920x1080, text on a wallpaper). `vga/modes/` lists them all, smallest first: `mode 1280x720` picks one, Ctrl+Up / Ctrl+Down step bigger / smaller. `vga/palette/` holds the colour themes.
* `font/`: the UzOS font (based on Terminus) and UzOS's own icons (folder, check, crescent...).
* `gui/`: video card (`bga`), drawing (`canvas`), double buffering (`screen`) and windows (`wm`).
* **Commands**: `date`, `devices` (IRQ lines and interrupt counts), `startx` (Ctrl+Up / Ctrl+Down resize it, ESC returns to the shell), `beep`, `play`, and for the screen `theme`, `font`, `mode`.

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
* **ELF Loader (`core/elf/`)**: Reads an ELF file from the filesystem, checks it, maps it into memory, and creates a process for it (the `exec` command, ring 0).
* **Signals (`core/signal/`)**: one bit per signal, numbered as in Linux (`kill -l`). On every return to ring 3 the kernel *delivers* the first one that is not blocked: it builds a frame on the program's own stack and makes the `iret` land in the program's **handler**; the handler's `ret` runs `rt_sigreturn`, which puts every register back. No handler: the default action (end, stop, ignore). `SIGSTOP`/`SIGCONT` freeze and thaw any process (state `T`); `rt_sigqueueinfo` sends a value too; `alarm` and `setitimer` (REAL / VIRTUAL / PROF) send signals on time.
* **Exit records and pidfds (`core/exitlog/`, `core/pidfd/`)**: the reaper keeps how each process ended, so `wait4`/`waitid` can give the parent its exit status later; a **pidfd** is a file descriptor that names one process for ever, so a signal can never reach "the next process with that number".
* **Jobs**: Ctrl+Z stops the foreground program; `jobs`, `fg`, `bg`, `wait` manage the shell's children; `logout` hangs up the session (SIGHUP) except for `nohup` / `setsid` jobs; `timeout` gives a command a deadline.
* **Sandboxes and debugging**: `prctl` (name, pdeathsig, subreaper, dumpable, no_new_privs), `seccomp` (strict, or a list of allowed syscalls, checked before every call), `ptrace` (attach, peek, poke, registers; Yama: only an ancestor), `process_vm_readv`, `strace`, `pmap`, `procinfo`, `pstree`, `pgrep`/`pkill`/`killall`/`pidof`.
* **vfork + execve**: `vfork()` starts a child that borrows the parent's memory and stack while the parent waits; `execve()` replaces the child's program (same pid) with an ELF file, builds `argc / argv / envp` on a new stack the System V way, and always starts it in **ring 3**. `fork()` needs a copy of memory, so it waits for per-process address spaces.
* **System calls (`syscalls/`)**: `int 0x80` with the number in EAX. `syscalls/handler/` routes every module's syscalls; this module's own are numbers 1, 3 and 52-96 (`getpid` and `sched_yield` live in cpu_management). `proccalls` calls them from ring 3 programs and checks 214 answers.
* **Security**: 39 attack/defence pairs in `security/<attack>/defence/<defence>/`, from `sigreturn_forgery` (SROP) and `pid_reuse_race` to `syscall_abuse` (seccomp) and `stack_clash`.

### E. CPU Management (`modules/cpu_management/`)
See [modules/cpu_management/README.md](modules/cpu_management/README.md) for the full map.
* **Info (`core/info/`)**: decodes `cpuid`: vendor, family/model/stepping, and feature flags (`tsc`, `pae`, `sse`, ...). The kernel asks `cpu_has(CPU_FEAT_TSC)` before it uses a feature.
* **Faults (`core/faults/`)**: a handler for every CPU exception. A fault in ring 3 (divide by zero, `cli`, `ud2`, ...) kills that process with a `[CPU FAULT]` report; a fault in ring 0 is a kernel bug and halts.
* **TSC (`core/tsc/`)**: the cycle counter, and the clock speed measured against the PIT.
* **Accounting (`core/accounting/`)**: every timer tick is charged to the process it interrupted (user, nice, system or idle); every task switch is timed exactly with the TSC; the last 60 seconds are kept. **Load average** (`core/loadavg/`): 1, 5 and 15 minutes, as in Linux.
* **Priority and policy (`core/priority/`, `core/policy/`)**: users set a **nice** value (-20..19), which becomes one of seven levels; equal levels take turns, 50 ms each, and **aging** stops starvation. Policies `SCHED_OTHER`, `SCHED_BATCH`, `SCHED_IDLE`, and real-time `SCHED_FIFO` / `SCHED_RR` (1..99), which may use at most 95% of each second (RT throttling).
* **Affinity, limits, NUMA (`core/affinity/`, `core/rlimit/`, `core/numa/`)**: CPU masks (one CPU), resource limits (CPU seconds, open files, file size, heap, processes, nice, real-time) and memory policies (one node). A new process inherits all of them (`core/task/`).
* **Threads (`syscalls/clone/`, `syscalls/clone3/`)**: make a thread that shares memory and starts in ring 3 as a copy of its parent (`clone_enter.s`); `syscalls/exit_group/` ends them all.
* **Perf**: the timer samples EIPs for `perf record` (`core/perf/`); counters you read through a file descriptor live in `syscalls/perf_event_open/`; `commands/perf_report/` knows the kernel's function names (the Makefile links the kernel twice for that).
* **Clean structure**: cpu_management has only `commands/`, `syscalls/`, `security/` and `core/`. Each command/syscall/attack/defence folder holds everything it needs (helpers are copied, several files allowed); `core/` keeps only what the kernel itself calls or state that must exist once. See `modules/cpu_management/core/README.md`.
* **/proc** (`filesystem_management/core/procfs/`): `cat /proc/cpuinfo`, `/proc/stat`, `/proc/<pid>/status`: files the kernel writes when you read them, built from cpu_management's numbers.
* **Syscalls** (one folder each): `getpid`, `gettid`, `sched_yield`, `cputime`, `setpriority`, `getpriority`, `sched_setscheduler`, `sched_getscheduler`, `sched_setparam`, `sched_getparam`, `sched_get_priority_min/max`, `sched_rr_get_interval`, `sched_setaffinity`, `sched_getaffinity`, `sched_setattr`, `sched_getattr`, `getrlimit`, `setrlimit`, `prlimit64`, `getcpu`, `perf_event_open`, `clone`, `clone3`, `move_pages`, `mbind`, `set_mempolicy`, `get_mempolicy`, `migrate_pages`, `membarrier`, `exit_group`. `cpucalls` tests them all from ring 3.
* **Commands** (one folder each): `lscpu`, `nproc`, `top`, `htop`, `btop`, `mpstat`, `vmstat`, `pidstat`, `sar`, `ps`, `uptime`, `nice`, `renice`, `chrt`, `taskset`, `numactl`, `time`, `perf stat|record|report|list`, `cpupower`, `turbostat`, `systemd-cgtop`, `systemctl`, `powerprofilesctl`, `ulimit`, `getconf`, and UzOS's own `cpuinfo`, `faults`, `cpustat`, `cpucalls`.

### F. User Management (`modules/user_management/`)
See [modules/user_management/README.md](modules/user_management/README.md) for the full map.
* **Users (`core/users/`)**: the user table: uid, name, home directory, and the password as a salt + SHA-256 hash (never the password itself). uid 0 is root; uids are never reused.
* **Hash (`core/hash/`)**: SHA-256, written out step by step.
* **Session (`core/session/`)**: the login prompt and switching users. Logging in sets the shell process's uid; every check in the kernel then sees that user.
* **Permissions (`core/perm/`)**: each file has an owner and a mode like `64` (`rw/r-`: owner reads and writes, others read). The filesystem syscalls ask before every open, create, delete, rename and `cd`.
* **Syscalls**: `getuid`, `setuid` (only CAP_SETUID, that is root, may change the uid).
* **Commands**: `whoami`, `id`, `users`, `su`, `logout`, `passwd`, `chmod`, `chown`, `useradd`, `userdel`. `ls` shows owners and modes; the prompt ends in `#` for root.

### G. Security Management (`modules/security_management/`)
See [modules/security_management/README.md](modules/security_management/README.md) for the full map. Two questions: **protection** (who may do what?) and **security** (what was tried, and is the kernel still itself?).
* **Capabilities (`core/caps/`)**: root's rights, split into named privileges like Linux has: `CAP_KILL`, `CAP_SETUID`, `CAP_FILES`, `CAP_CHOWN`, `CAP_USERS`, `CAP_PRIORITY`, `CAP_RESOURCE`, `CAP_SECLAB`. Every check asks for exactly one (`caps_has(uid, CAP_KILL)`), and one table decides who holds which: root all, other users none.
* **Audit log (`core/audit/`)**: every refusal of every defence, with the time, the user and the PID. A ring of the newest 32 events, plus a counter per defence that never forgets. No one can clear it.
* **Kernel integrity (`core/integrity/`)**: SHA-256 of `.text` and `.rodata` at boot; `exec` measures again before it starts a program and refuses on a patched kernel.
* **Security lab (`core/registry/`, `core/catalog/`, `core/watchdog/`)**: the list of every module's attack/defence pairs, and the `seclab` command that runs them. A live run is BLOCKED only when the attack's own defence refused it.
* **Syscalls**: `getcaps` (which capabilities do I have?).
* **Commands**: `audit [sum]`, `caps [user]`, `integrity`, `seclab`.
* **Its own pairs**: `kernel_patch` ↔ `integrity_check` (one byte written into kernel code), `log_flood` ↔ `audit_limit` (hiding a refusal under a flood of cheap ones).

### H. Filesystem Management (`modules/filesystem_management/`)
See [modules/filesystem_management/README.md](modules/filesystem_management/README.md).
* **VFS (`core/vfs/`)**: the tree of files and directories, path lookup (`/a/b`, `../x`), and open counts.
* **RAMDisk (`core/ramdisk/`)**: the driver that keeps every file's bytes in the kernel heap.
* **Commands**: `ls cd pwd mkdir touch rm cat write cp mv wc grep hexdump`, all with full paths. Every file has an owner and a mode (see User Management).

### I. Shell (`modules/shell/`)
See [modules/shell/README.md](modules/shell/README.md).
* **Core**: the command registry, the line being typed (`line/`), history (Up/Down) and Tab completion for commands and paths.
* **Commands**: `help` (built from the registry), `clear`, `echo`, `color`, `calc` (a recursive-descent parser), `history`, `snake`.
* **Prompt**: `guest@uzos:~/docs$`, like a Linux shell; `#` instead of `$` for root.
* **Colours while you type (`core/highlight/`)**: the command word turns green when it is a real command, red when no command starts like it.
* **Ctrl+C**: at the prompt it clears the line; while a command runs it sends the signal `SIGINT` (`process_management/core/signal/`), and `sleep`, `top`, `exec`, `snake`, `startx` stop early. `exec /hello` runs in the foreground; `exec /hello &` in the background. Commands that start another command (`nice`, `time`, `perf stat`, ...) pass Ctrl+C on to it.

### J. UI (`modules/ui/`)
See [modules/ui/README.md](modules/ui/README.md).
* **Style (`core/style/`)**: one look for every message: `ui_error`, `ui_ok`, `ui_warn`, `ui_usage`; colours by meaning, not by number.
* **Frames and bars (`core/box/`)**: `┌─ Title ─┐` panels, table cells that cannot be broken by a strange name, `███░░` bars. `meminfo`, `heapinfo`, `ps`, `devices` and `ioinfo` use them.
* **Boot log, logo, status bar**: the `[  OK  ]` lines, the banner and flag, and the top bar kept fresh by the `statusd` daemon.
* **Commands**: `uzfetch` (a system summary next to the flag), `statusbar [on|off]`.

### K. Network Management (`modules/network_management/`)
See [modules/network_management/README.md](modules/network_management/README.md) for the full map and the life of a packet.
* **The card (`core/pci/`, `core/e1000/`)**: the PCI bus is scanned at boot; the Intel e1000 (QEMU's and VirtualBox's default) gets its registers mapped, two rings of DMA descriptors, and an interrupt. `ethtool -t` makes it send a frame to itself.
* **Interfaces and packets (`core/netdev/`, `core/loopback/`, `core/packet/`)**: `eth0` and `lo`; a packet buffer has room in front, so each layer pushes its header on the way down and pulls it off on the way up (Linux's sk_buff idea).
* **The protocols (`core/ethernet/`, `arp/`, `ipv4/`, `route/`, `icmp/`, `udp/`, `tcp/`)**: Ethernet frames, ARP (IP -> MAC), IPv4 with fragments and a routing table, ICMP (ping, errors), UDP ports, and TCP: the three-way handshake, sequence numbers, retransmission with measured round-trip times, FIN and TIME_WAIT.
* **Sockets (`core/socket/`, `core/unix/`, `core/poll/`, `core/epoll/`)**: a socket is a file descriptor (`read`/`write`/`close` work on it); UDP, TCP, raw and AF_UNIX sockets; a blocked call sleeps until the network wakes it; `select`, `poll` and `epoll` wait on many at once.
* **Getting on the network (`core/dhcp/`, `core/dns/`, `core/hostname/`)**: DHCP at boot gives 10.0.2.15 under QEMU; the resolver reads `/etc/hosts`, keeps a cache and asks the DNS server; `ping example.com` reaches the real internet.
* **The firewall (`core/firewall/`)**: INPUT and OUTPUT chains of rules (`iptables`), checked after fragments are put together.
* **Commands** (33): `ip`, `ifconfig`, `route`, `arp`, `dhclient`, `ping`, `traceroute`, `tracepath`, `mtr`, `arping`, `host`, `nslookup`, `dig`, `hostname`, `nc`, `telnet`, `curl`, `wget`, `httpd` (open http://localhost:8080 on the host), `echod`, `netstat`, `ss`, `tcpdump`, `iftop`, `nload`, `nstat`, `iptables`, `nmap`, `lspci`, `ethtool`, `ifup`, `ifdown`, and `netcalls` (68 syscall answers from ring 3).
* **System calls** (numbers 133-166): `socket`, `bind`, `connect`, `listen`, `accept4`, `sendto`, `recvfrom`, `sendmsg`, `recvmsg`, `setsockopt`, `socketpair`, `socketcall`, `select`, `poll`, `epoll_*`, `ioctl`, `sendfile`, `uname`, `sethostname`, ...; errors are Linux's `-errno`.
* **Security**: 47 attack/defence pairs, from `arp_spoofing` and `ping_of_death` to `syn_flood` (SYN cookies), `rst_injection` (RFC 5961) and `dns_spoofing`. The attacks inject crafted frames as if they came off the wire (`netdev_inject`), so every defence is met in the real packet path.

---

## 5. How to Contribute / Extend

### Adding a new Shell Command
Every module follows the same pattern:
1. **Create the folder**: `modules/<module>/commands/<mycmd>/cmd_<mycmd>.c`, starting with a short comment: what it does, what it uses, why.
2. **Declare it** in the module's `commands/<module>_commands.h` (cpu_management: in its own `cmd_<mycmd>.h`, next to the .c; include files by their full path, e.g. `#include "modules/cpu_management/core/tsc/tsc.h"`).
3. **Register it**: add one line to the command table in `commands/<module>_commands.c`.

### Adding a new System Call
1. **Create the folder**: `modules/<module>/syscalls/<name>/` with `sys_<name>.h` (the `SYS_<NAME>` number and the prototype) and `sys_<name>.c`.
2. **Route it**: include `sys_<name>.h` in `modules/process_management/syscalls/handler/syscall_handler.c`, write a small `do_<name>()` wrapper (check pointer arguments with `buffer_ok` / `path_ok`), and add one line to the table.
3. Arguments come in EBX, ECX, EDX, ESI, EDI, EBP; the wrapper's return value goes back to the program in EAX.

### Printing from a command
Use the ui module, not raw colours: `ui_error("mycmd", "what went wrong")`, `ui_usage("mycmd <arg>")`, `ui_ok(...)`. For a table, `box_begin` / `box_row` / `ui_cell` / `box_end`.

Enjoy exploring UzOS! Start by modifying `kernel_main.c` or adding a new command to see your code come to life.
