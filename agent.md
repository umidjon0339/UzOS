# Agent State Tracker

## Current Phase
Phase 5: User Mode, System Calls, and Task Scheduling.
Phase 5.5: Process Security Lab (attacks + defences) - **done for Tier 1**.
Phase 6: CPU management (`modules/cpu_management/`) - **done**: info,
fault policy, TSC, accounting, priority scheduling with aging.
Phase 6b: Linux CPU tools and syscalls in cpu_management - **done**:
30 commands and 30 syscalls, one folder each, full-path includes;
nice + policies (FIFO/RR/BATCH/IDLE, RT throttling), affinity, rlimits,
NUMA policies (one node), clone threads, perf counters + sampling with
kernel symbols (two-pass link), /proc, power profiles, load average.
`cpucalls` = 63 of 63 syscall answers from ring 3.
Clean structure: cpu_management = commands/ syscalls/ security/ core/
only. Each command/syscall/attack/defence folder is self-contained
(helpers copied: run_child.c in the 7 launcher commands, own thread code
in clone/clone3/exit_group, perf counters in syscalls/perf_event_open,
ksyms in commands/perf_report). core/ keeps only what the kernel calls
or shared state (rule in cpu_management/core/README.md). /proc moved to
filesystem_management/core/procfs.
Phase 7: User management (`modules/user_management/`) - **done**: user
table with salted SHA-256 passwords, login prompt, su/passwd, file
owners and modes, useradd/userdel.
Phase 8: Security management (`modules/security_management/`) - **done**:
the security lab moved and split (registry, catalog, audit, watchdog),
capabilities, audit log with user/PID, kernel integrity (SHA-256 of
.text/.rodata), getcaps syscall, two pairs of its own.

## Implemented Modules
- Multiboot boot (`boot/boot.s`), linker script (`boot/linker.ld`), Makefile
- `arch/x86/` split into one folder per feature: io, cpu, cpuid, gdt, tss, idt, isr, usermode
- Build writes objects to `build/` (git-ignored); `lib/string/` is the kernel's C string library
- `modules/core/`: `kernel_main.c` as a step-by-step table of contents with a Linux-style boot log
- Console (`device_management/vga/`) over a display interface: VGA text 80x25 / 80x50,
  1024x768 graphics console (`gui/fbcon/`); status row; full-screen mode
- UzOS font (Terminus, OFL) + icons (`device_management/font/`), 6 colour themes (`vga/palette/`)
- RTC driver (`device_management/rtc/`), `date`, `time` syscall (19)
- `modules/ui/`: message styles, frames/bars/cells, boot log, logo, status bar + statusd daemon, `uzfetch`
- GDT (flat kernel + user segments, TSS) and `set_kernel_stack()`
- IDT, 31 CPU exception ISRs, 16 IRQs, PIC remapping, page fault handler
- PIT at 100 Hz, PS/2 keyboard, PS/2 mouse, PC speaker
- PMM bitmap frame allocator, VMM 2-level paging (CR3/CR0), kernel heap
- VFS over a RAMDisk; `ls cd pwd cat touch mkdir write rm cp mv wc grep hexdump`
- Processes (PCB table), scheduler + context switch, ELF32 loader,
  `core/usertask/` runs small .s programs in ring 3 (testuser, attacks)
- `modules/cpu_management/`: cpuid features (`cpuinfo`, `lscpu`), a handler for
  every CPU exception (`faults`), TSC clock speed, CPU time per process
  (exact task clock per switch, 60 s history, load average), nice -> levels
  1-7 with 50 ms slices and aging, policies OTHER/BATCH/IDLE/FIFO/RR with
  RT throttling, CPU affinity, resource limits (enforced: CPU, FSIZE, DATA,
  NPROC, NOFILE, NICE, RTPRIO), NUMA policies, clone()/clone3() threads,
  perf_event_open + perf record/report with kernel symbols, power
  profiles; /proc (cpuinfo, stat, <pid>/status) in filesystem_management. Syscalls 2, 4, 20-22,
  26-51. Commands: lscpu nproc top htop btop mpstat vmstat pidstat sar ps
  uptime nice renice chrt taskset numactl time perf cpupower turbostat
  systemd-cgtop systemctl powerprofilesctl ulimit getconf cpuinfo faults
  cpustat cpucalls. `ps`, `top` (from process_management), `uptime` (from
  device_management), syscalls `getpid` and `yield` (now `sched_yield`)
  moved here; each command/syscall has its own folder and header
- `modules/user_management/`: users root (0) / guest (1) / 1000+ via
  `useradd` (uids never reused), passwords as salt + SHA-256 x1000, login
  prompt at boot, `su`, `logout`, `passwd`, file owner + mode (`chmod`,
  `chown`, checked in fs/io syscalls and `vfs_lookup`), syscalls 23/24
- `modules/security_management/`: capabilities (`caps`, CAP_KILL ... CAP_SECLAB,
  `getcaps` syscall 25) replace every privilege `uid == 0` test; audit log
  (`audit [sum]`: ring of 32 + per-defence counters, user + PID, rate limit);
  kernel integrity (`integrity`, checked before every `exec`); the lab
- Shell: history, Tab completion, `guest@uzos:~$` / `root@uzos:~#` prompt, 83 commands (table: 128),
  command word coloured while typing (green / normal / red), Ctrl+C
  (clears the line, or SIGINT to the running command: `core/signal/`,
  `sys_sleep_interruptible`); `exec` runs in the foreground, `&` = background
- GUI: BGA framebuffer, compositing window manager, `startx`

## Security Lab
Shared framework: `modules/security_management/core/` (registry,
catalog, audit, watchdog) and the `seclab` command. Each module keeps its
own pairs in `modules/<module>/security/{attack,defence}/<id>/`.

Drive it with `seclab list | info <id> | attack <id> [live] |
defence <id> | verify <id|all> | log | status | repair`. A live run is
BLOCKED only when the attack's own defence refused something; run
`seclab repair` between live runs (fork_bomb's workers fill root's quota).
40 pairs, `seclab verify all` = 40 of 40.

process_management pairs (8):
1. `fork_bomb`            <-> `slot_quota`   per-user cap + root reserve
2. `kill_system`          <-> `kill_guard`   system processes unkillable, own-only kills
3. `stack_overflow`       <-> `stack_guard`  real ESP checked on every switch
4. `zombie_leak`          <-> `reaper`       zombies freed on the timer tick
5. `pid_wrap`             <-> `pid_bounds`   PID counter stops before wrapping
6. `uninit_pcb`           <-> `pcb_wipe`     slot zeroed before reuse
7. `kernel_stack_to_user` <-> `user_stack`   ring 3 only gets a user-space ESP
8. `malicious_elf`        <-> `elf_validate` ELF header/segments checked

memory_management pairs (7):
1. `user_ptr_wrap`    <-> `user_ptr_bounds` ranges checked without wrap
2. `mmap_length_wrap` <-> `mmap_page_count` length limit, safe rounding
3. `pmm_bad_free`     <-> `pmm_free_guard`  no kernel/bogus/double frame frees
4. `heap_double_free` <-> `heap_guard`      kfree only real, used blocks
5. `heap_overflow`    <-> `heap_canary`     magic checked before following
6. `munmap_kernel`    <-> `mmap_ownership`  munmap only mmap's own regions
7. `null_deref`       <-> `null_page`       page 0 unmapped

device_management pairs (7):
1. `timer_divide_by_zero`    <-> `timer_frequency`   PIT rate checked before dividing
2. `mouse_coordinate_escape` <-> `mouse_bounds`      pointer clamped to the screen
3. `mouse_desync`            <-> `mouse_packet_sync` resync on bit 3, drop overflow
4. `irq_storm`               <-> `irq_mask`          only lines with a driver open
5. `canvas_overflow`         <-> `canvas_clip`       every rectangle clipped
6. `rtc_garbage`             <-> `rtc_validate`      only real dates leave the RTC driver
7. `invisible_text`          <-> `contrast_check`    text colours readable on their background

filesystem_management pairs (4):
1. `bad_filename`     <-> `filename_check` component length, no ./../'/'/spaces
2. `file_offset_wrap` <-> `file_range`     offset + size without wrap
3. `delete_open_file` <-> `busy_check`     no delete while open/non-empty/cwd
4. `dir_cycle`        <-> `no_cycle`       no directory moved inside itself

io_management pairs (3):
1. `invalid_descriptor` <-> `descriptor_bounds` real, open, right-mode fds
2. `oversized_transfer` <-> `transfer_bounds`   NULL/huge/wrapping buffers
3. `fd_leak`            <-> `fd_cleanup`        close files when a process ends

shell pairs (2):
1. `calc_overflow` <-> `safe_arith`  64-bit steps, /0 and INT_MIN/-1 refused
2. `line_overflow` <-> `input_bound` typed line never exceeds its buffer

ui pairs (1):
1. `output_spoof` <-> `safe_text`  names in tables: no control characters, cut to fit

user_management pairs (2):
1. `setuid_escalation`      <-> `setuid_guard` only CAP_SETUID may change the uid
2. `file_permission_bypass` <-> `file_access`  owner + mode checked; CAP_FILES passes

cpu_management pairs (4):
1. `user_exception`        <-> `fault_isolation` a ring 3 fault kills only that process
2. `tsc_timing`            <-> `tsc_restrict`    CR4.TSD: rdtsc is kernel-only
3. `priority_out_of_range` <-> `priority_bounds` levels 1-7, users only lower their own
4. `cpu_hog`               <-> `aging`           waiting raises priority: no starvation

security_management pairs (2):
1. `kernel_patch` <-> `integrity_check` SHA-256 of kernel code at boot vs before exec
2. `log_flood`    <-> `audit_limit`     10 events per defence per second; the rest counted

Tier 2 (needs per-process paging): cross-process memory read/write,
guard pages below stacks, TOCTOU on the kill path.

## Phase 5 - remaining work
- [x] TSS setup (`arch/x86/tss/tss.c`)
- [x] `set_kernel_stack()` called on every context switch
- [x] Ring 3: `testuser` maps a user code + stack page and enters ring 3;
      the program calls getpid/mmap/munmap/exit (`testuser null` faults)
- [x] System Call Interface: `int 0x80` dispatch in
      `process_management/syscalls/handler/` (exit, getpid, sleep, yield,
      sbrk, mmap, munmap)
- [x] Round-robin preemptive scheduler; EFLAGS saved per task
- [ ] `fork()`: removed until per-process page directories exist
- [ ] Per-process page directory and CR3 switch
- [ ] `wait()` for user processes

## Known Bugs / Issues
Fixed by the security lab:
1. `process_fork()` leaked a slot + 4 KB stack on every failure - `fork_cleanup`
2. `process_create()` left `cwd`, `fd_table`, `fd_offset` uninitialised - `pcb_wipe`
3. `process_kill()` freed the running process's own stack - `defer_self_kill`
4. PID-1 protection lived in the shell command, not the kernel - `kill_guard`
5. `child_count` was never decremented - `child_release`
6. `process_exit()` spun forever and never returned its slot - `reaper`
7. `proc_security_check_stack_overflow` and `set_kernel_stack` had no callers - `stack_guard`
8. `testuser` handed ring 3 the kernel's own ESP - `user_stack`
9. Stale `.o` files with no source left over from an earlier refactor - deleted

Fixed in boot/arch:
10. `int 0x80` arrived as `int_no = 0xFFFFFF80` (`push byte 128`), so
    syscalls were silently dropped - stubs now push dwords
11. TSS limit was `base + size` and `iomap_base` was 0, letting ring 3 use
    ports read from the TSS's own bytes - limit fixed, bitmap moved past it
12. TSS `esp0` was 0 until the first context switch - now the boot stack
13. Interrupt stubs never cleared DF; boot entry never cleared DF
14. Spurious IRQ7/IRQ15 got a wrong EOI
15. Linker script let `.rodata.*`, `.eh_frame`, build-id notes through as
    orphans; `_kernel_end` was not page aligned

Fixed in process_management restructure:
16. `switch_task` did not save EFLAGS: after `sleep`/`top`/`spawn` the shell
    resumed with IF=0 and the next `hlt` froze the OS
17. `def_elf_validate`, `def_user_stack_ok`, `def_child_release` were stubs
18. kill_guard protected PID 1 (idle) but not kernel_main (the shell)
19. `sys_exit` never retired the process; `sys_getpid` always returned 0
20. `int 0x80` handler ignored the syscall number
21. ELF loader mapped page 0 when out of memory and ignored read errors
22. Lab registry held 16 entries; 17 are registered now (limit 32)
23. `top` missed 'q' when another key followed it

Fixed in memory_management restructure:
24. `vmm_map_page` wrote new page tables through their PHYSICAL address;
    frames above 8 MiB page-faulted (`exec`, `testuser`). Now a recursive
    page directory. It also returns errors, sets PAGE_USER on the table
    for user pages, flushes the TLB, and frees empty page tables
25. kmalloc/kfree, pmm alloc/free were not interrupt-safe (reaper)
26. "user pointer" check treated everything below 0xC0000000 as user,
    including the kernel at 1 MiB; munmap could free kernel frames
27. mmap returned raw physical addresses (unmapped above 8 MiB); sbrk
    returned kmalloc memory. Now real user pages; wired into int 0x80
28. PMM freed partly-reserved frames, let available regions override
    reserved ones, never reserved multiboot data/modules
29. pmm_free accepted kernel frames and double frees
30. page 0 was mapped: NULL accesses were silent
31. any page fault halted the OS; a ring 3 fault now kills that process
32. kmalloc_a returned pointers kfree could not free (removed, unused)

Fixed in device_management restructure:
33. mouse handler sent EOI twice; packet decoder never resynchronised
34. all 16 IRQ lines were unmasked at boot (now only lines with a driver)
35. startx could never exit; leaked its 3 MiB back buffer and windows;
    exit now restores VGA text mode registers and the saved font
36. window text drawing wrote before the buffer for negative x/y
37. "System Monitor" showed hard-coded numbers (now live)
38. vga_print_int(INT_MIN) overflowed; VGA output was not interrupt-safe
39. timer_wait broke at tick wrap-around; sleep/uptime hard-coded 100 Hz
40. font table was missing '~' (127 of 128 entries)

Fixed in filesystem + io restructure:
41. rm freed open files / non-empty dirs / the cwd: use after free
42. paths were single names only; open("/x") looked in the cwd
43. write did not truncate ("hi" over "hello" gave "hillo"); cat stopped
    at 255 bytes; mv skipped name checks; no seek
44. files that a dying process left open were never closed
45. io/fs syscalls were not reachable from ring 3; ring 3 pointers are
    now checked for being mapped user memory (a bad one used to fault
    inside the kernel)
46. each file took 16 KiB on first write (now grows from 256 bytes)

Fixed in shell restructure:
47. calc: -2147483648 / -1 and x / 0 raised CPU exception 0 (kernel halt);
    silent overflow; '%%' shown literally; now a precedence parser
48. help was a stale hard-coded list (now built from the registry)
49. leading/extra spaces broke command names and arguments
50. Down arrow erased the typed line when not browsing history
51. beep/play printed UTF-8 junk; speaker code moved to device_management
52. snake: two quick keys reversed into itself; food could land on the
    snake; max length corrupted it; quitting wiped the screen

Fixed in the look-and-feel step:
53. `color` lasted only until the next message (every command reset to
    white); it is now the normal text colour, and unreadable pairs are refused
54. `top` cleared the scroll-back history (vga_init) on start and on quit;
    it now borrows the screen and gives it back
55. a kernel panic during snake/top/startx printed into a hidden screen;
    panics now take the screen back first
56. printing while a full-screen program ran wrote over it (e.g. the spawn
    worker's heartbeat over snake); text now goes to history until it ends
57. ps/top printed process names raw: a newline in a name faked a row
    (safe_text)
58. the prompt could start in the middle of a line after output without '\n'
59. help wrapped at a fixed 80 columns; it now uses the screen width

Fixed in cpu_management:
60. only page faults had a handler: any ring 3 `div 0`, `ud2`, `cli`,
    `hlt` or `in` reached the panic and halted the whole OS
    (fault_isolation)
61. the idle task was always READY, so round robin gave it a turn every
    cycle even while others had work (it now has its own lowest level)
62. the shell waited for keys with `hlt` while staying READY: it looked
    busy to the scheduler and to CPU accounting (it now sleeps 10 ms)
63. ring 3 could read the cycle counter for timing attacks (tsc_restrict)
64. the ring 3 launcher lived inside `testuser`; moved to
    `process_management/core/usertask/` so attacks can share it

Fixed in user_management:
65. the shell was always root and processes got fixed uids (spawn 0,
    exec 1): there was no way to be anyone else
66. any process could read, change or delete any file: files had no owner
67. `spawn` and `exec` now start processes as the user who asked

Fixed in the shell step:
68. nothing could stop a running command: `sleep 30` had to run out, a
    program that never exits (`exec hello`) needed `kill`
69. a password cancelled halfway (Esc) stayed on the stack

Fixed in security_management:
70. a live run counted ANY refusal as "BLOCKED": six ring 3 attacks run
    after fork_bomb were really refused process slots by slot_quota and
    never ran, yet showed BLOCKED. The verdict now counts only the
    attack's own defence
71. the audit log did not say who was refused (no uid, no PID), could be
    read half-written by an interrupt, and one noisy defence could push
    every other event out of it (audit_limit)
72. privilege was a `uid == 0` test copied into a dozen checks; now one
    capability table (core/caps)
73. nothing noticed a write into the kernel's own code (integrity_check)
74. `process_management/README.md` said ELF programs run as uid 1; they
    run as the user who started them

Still open:
- Kernel code pages are writable (identity map with PAGE_WRITABLE, CR0.WP
  off): integrity_check detects a patch but cannot prevent it.
- Capabilities follow the uid; there is no per-process capability set,
  so a process cannot drop one and keep the rest.
- No groups (Linux's middle mode digit) and no execute bit: read is
  enough to `exec` a program.
- `userland/hello.c` (`exec hello`) runs in ring 0 and ends in a `hlt`
  loop instead of exiting: it stays at 100% CPU in `cpustat` until killed.
  It needs an exit syscall and a ring 3 ELF loader.
- One global `current_directory`: no per-process address space isolation.
- `def_stack_guard_check` is a software check. The stronger fix is a guard
  page below each stack, which needs per-process paging.
