# Agent State Tracker

## Current Phase
Phase 5: User Mode, System Calls, and Task Scheduling.
Phase 5.5: Process Security Lab (attacks + defences) - **done for Tier 1**.

## Implemented Modules
- Multiboot boot (`boot/boot.s`), linker script, Makefile
- VGA text mode driver (colour, strings, ints, hex, scrolling, cursor)
- GDT (flat kernel + user segments, TSS) and `set_kernel_stack()`
- IDT, 31 CPU exception ISRs, 16 IRQs, PIC remapping, page fault handler
- PIT at 100 Hz, PS/2 keyboard, PS/2 mouse, PC speaker
- PMM bitmap frame allocator, VMM 2-level paging (CR3/CR0), kernel heap
- VFS over a RAMDisk; `ls cd pwd cat touch mkdir write rm cp mv wc grep hexdump`
- Processes (PCB table), round-robin scheduler, context switch, ELF32 loader
- Shell: history, Tab completion, ~30 commands
- GUI: BGA framebuffer, compositing window manager, `startx`

## Process Security Lab
`modules/process_management/security/` holds paired attack/defence modules.

```
security/
├── attack/<id>/atk_<id>.c      # how the guarantee is broken
└── defence/
    ├── core/sec_core.c         # registry, event logger, watchdog
    └── <id>/def_<id>.c         # how the break is stopped, called by the kernel
```

Drive it from the shell with `seclab list | info <id> | attack <id> [live] |
defence <id> | verify <id|all> | log | status | repair`.

Core 6 Attack/Defence Pairs (Educational & Essential):
1. `fork_bomb`       <-> `slot_quota`   (Resource exhaustion vs Per-user quota & root reserve)
2. `kill_init_bypass`<-> `kill_guard`   (PID 1 termination vs System process protection)
3. `stack_overflow`  <-> `stack_guard`  (Kernel stack smashing vs Stack canary validation)
4. `zombie_leak`     <-> `reaper`       (Unreaped zombie buildup vs Auto-reaping daemon)
5. `pid_wrap`        <-> `pid_bounds`   (Out-of-bounds/wrap PIDs vs Range validation)
6. `uninit_pcb`      <-> `pcb_wipe`     (Stale memory leak vs PCB zeroing on allocation)

Tier 2 (needs per-process paging / a real ring 3 exit path):
cross-process memory read/write, TSS `esp0` races, IOPL and port I/O from
ring 3, fd bounds checks, TOCTOU on the kill path.

## Phase 5 - remaining work
- [x] TSS setup (`arch/x86/gdt.c`)
- [x] `set_kernel_stack()` called on every context switch
- [~] Ring 3 switch: assembly exists (`switch_to_user_mode`), but there is no
      user stack allocation, so `testuser` is now correctly refused
- [ ] System Call Interface (`int 0x80`): the handler still only prints the
      syscall number. All `sys_*` functions are plain kernel calls, not syscalls
- [x] Round-robin preemptive scheduler
- [ ] `process_fork()`: limits and cleanup are done, but there is no page
      directory or register copy yet, so it returns -1 by design
- [ ] Per-process page directory and CR3 switch
- [ ] Real `sys_exit` / `wait` path for user processes

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

Still open:
- `arch/x86/interrupt.s:60` does `push byte 128`, which NASM truncates, so
  `int 0x80` arrives as -128 instead of 128. Needs `push dword 128`.
- The `int 0x80` handler has no dispatch table.
- One global `current_directory`: no per-process address space isolation.
- `def_stack_guard_check` is a software check. The stronger fix is a guard
  page below each stack, which needs per-process paging.
