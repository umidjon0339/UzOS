# process_management — processes, scheduling, system calls

A **process** is a function with its own stack that the CPU runs in
turns with the others. This module creates and ends processes, decides
who runs next, loads programs from files, and gives user programs a door
into the kernel (`int 0x80`).

```
process_management/
├── core/                         the machinery
│   ├── process/    process.h/.c       process table: create, kill, exit
│   ├── scheduler/  scheduler.h/.c     hand the CPU over (policy: cpu_management)
│   │               switch_task.s      swap the CPU between two processes
│   ├── elf/        elf.h/.c           load an ELF program file and start it
│   ├── signal/     signal.h/.c        pending signals (SIGINT = Ctrl+C)
│   └── usertask/   usertask.h/.c      run a small .s program in ring 3 (testuser, attacks)
├── commands/                     shell commands, one folder each
│   ├── proc_commands.h/.c             the command table
│   ├── kill/ spawn/ sleep/ exec/          (ps and top: cpu_management)
│   └── testuser/   cmd_testuser.c     run a program in ring 3 (via usertask)
│                   user_program.s     ...these programs
├── syscalls/                     one folder per system call: sys_<name>.h/.c
│   ├── handler/    syscall_handler.h/.c   int 0x80 -> the right sys_*()
│   ├── exit/ sleep/
│   ├── getppid/ setpgid/ getpgid/ getpgrp/ setsid/ getsid/   identity, groups, sessions
│   ├── kill/ tkill/ tgkill/ pidfd_send_signal/               signals (signal_permission gate)
│   ├── wait/ waitpid/ waitid/                                 wait for a child
│   └── fork/ vfork/ execve/ execveat/   STUBS: need per-process address spaces
│                                          (getpid, sched_yield, clone: cpu_management)
└── security/                     one folder per ATTACK; its DEFENCE nested inside
    └── <attack>/   atk_<attack>.c + README + defence/<defence>/def_<defence>.h/.c
                    (16 pairs; see security/README.md)
```

## Life of a process

```
            process_create()          scheduler picks it
   FREE  ------------------>  READY  -------------------->  RUNNING
    ^                           ^  <-------------------- /    |   |
    |                           |     timer: next turn  /     |   | sys_sleep()
    |                           +--- SLEEPING <--------+------+   |
    |                                 (timer counts down)         | returns, sys_exit(),
    |                                                             | or process_kill()
    +---------------- reaper (next timer tick) <------ ZOMBIE <---+
```

- Every process belongs to a user (`process_t.uid`). A new one gets the
  uid of whoever created it (`process_caller_uid()`); the shell's uid is
  the logged-in user (user_management). `ps` and `top` (cpu_management) show the name.
- `scheduler_init()` creates two **system** processes: `idle` (PID 1),
  which runs when nobody else can, and `kernel_main` (PID 2), the code
  that was already running, which is the shell. Neither can be killed.
- A process never frees its own stack, because it is standing on it. It
  becomes a `ZOMBIE`, and the reaper frees it on a later timer tick.

## Signals

A **signal** is a one-bit message to a process (`process_t.signals`).
Sending one changes nothing by itself; the process checks it when it
chooses. UzOS has one so far: `SIGINT` (2), sent by Ctrl+C.

| Sleep | Ends when |
|---|---|
| `sys_sleep(ms)` | the time is up, always (uninterruptible) |
| `sys_sleep_interruptible(ms)` | the time is up, or a signal arrives (returns 0) |

`signal_send()` runs in the keyboard interrupt. It sets the bit and
wakes the process if it is in an interruptible sleep. Both happen with
interrupts off, and the sleep checks for a signal and goes to sleep with
interrupts off too, so a Ctrl+C can never fall into the gap between them.

Short internal waits (usertask, the security lab's attacks) use the
uninterruptible sleep, so a Ctrl+C cannot cut them off halfway.

## How a task switch works

The timer fires 100 times a second → `scheduler_tick()` asks
cpu_management whether the turn is over (`priority_tick()`) → if so,
`scheduler_yield()` asks who is next (`priority_pick_next()`: highest
priority, equal levels take turns, aging so nobody starves) →
`switch_task()`:

1. push EFLAGS and the registers C must keep onto **our** stack
2. save `ESP` in our `process_t`
3. load the other process's `ESP`
4. pop **its** registers and EFLAGS, then `ret` into it

Every process that is not running is frozen inside `switch_task()`. A
brand-new process gets a stack built to look the same
(`scheduler_prepare_stack()`), so its first "return" lands in
`process_start()`, which calls its function.

## How a system call travels

```
user program:  mov eax, 2      ; SYS_GETPID
               int 0x80
                  |
arch/x86/isr:  isr128 stub -> isr_handler() -> registered handler for 128
                  |
syscalls/handler:  syscall_handler(regs): switch (regs->eax)
                  |
cpu_management/syscalls/getpid:   sys_getpid()  ->  regs->eax = PID
                  |
iret: back in the program, EAX = its PID
```

| # | Syscall | Argument (EBX) | Result (EAX) |
|---|---------|----------------|--------------|
| 1 | `exit`   | none          | never returns (ends this thread) |
| 3 | `sleep`  | milliseconds  | 0 |

Numbers 2 (`getpid`) and 4 (`sched_yield`, once called `yield`) moved
to [cpu_management](../cpu_management/syscalls/) with the other
scheduling calls; their numbers did not change.

The other numbers live with their own module, and the same handler's
table routes them all:

| # | Module |
|---|---|
| 5–7 | `sbrk`, `mmap`, `munmap`: [memory_management](../memory_management/syscalls/) |
| 8 | `uptime`: [device_management](../device_management/syscalls/) |
| 9–13 | `mkdir`, `unlink`, `chdir`, `getcwd`, `rename`: [filesystem_management](../filesystem_management/syscalls/) |
| 14–18 | `open`, `read`, `write`, `close`, `seek`: [io_management](../io_management/syscalls/) |
| 19 | `time`: [device_management](../device_management/syscalls/) |
| 2, 4, 20–22, 26–51 | `getpid`, `sched_yield`, `cputime`, `setpriority`, `getpriority`, the `sched_*` calls, limits, `clone`, NUMA, `perf_event_open`, `membarrier`, `exit_group`: [cpu_management](../cpu_management/README.md#system-calls) |
| 23–24 | `getuid`, `setuid`: [user_management](../user_management/syscalls/) |
| 25 | `getcaps`: [security_management](../security_management/syscalls/) |

For calls from ring 3, every pointer argument (buffer or path) is
checked first by `user_ptr_bounds`: it must be the program's own mapped
memory, so a bad pointer fails only that call.

## Commands

`ps` and `top` moved to [cpu_management](../cpu_management/README.md#commands).

| Command | What it does | Uses |
|---|---|---|
| `kill <pid>` | stop a process | `process_kill` (→ kill_guard) |
| `spawn` | start a worker that beats 3 times, then exits | `process_create`, `sys_sleep`, `sys_exit` |
| `sleep <s>` | pause the shell (Ctrl+C ends it) | `sys_sleep_interruptible` |
| `exec <file> [&]` | load and run an ELF program, e.g. `exec /hello`: in the foreground (Ctrl+C stops it), or with `&` in the background; refused if the kernel's code changed (integrity_check) | `elf_load_and_run`, `sys_sleep_interruptible`, `process_kill` |
| `testuser [null\|badptr]` | run a tiny ring 3 program: `write` (prints hello), `getpid`, `uptime`, `mmap`, `munmap`, `exit`; with `null` it writes to address 0 and only it is killed; with `badptr` it passes `write()` an unmapped buffer and only that call fails | usertask (VMM, `switch_to_user_mode`, user_stack) |

## Security: attack ↔ defence

| Attack | Breaks | Defence | Called from |
|---|---|---|---|
| `fork_bomb` | resource | `slot_quota` | `process_create` |
| `kill_system` | privilege | `kill_guard` | `process_kill` |
| `stack_overflow` | isolation | `stack_guard` | `scheduler_yield` |
| `zombie_leak` | resource | `reaper` | `scheduler_tick` |
| `pid_wrap` | identity | `pid_bounds` | `process_create`, `process_kill` |
| `uninit_pcb` | disclosure | `pcb_wipe` | `process_init`, `process_create` |
| `kernel_stack_to_user` | privilege | `user_stack` | usertask (`testuser`, cpu attacks) |
| `malicious_elf` | trust | `elf_validate` | `elf_load_and_run` |
| `signal_abuse` | privilege | `signal_permission` | signal-sending paths |
| `orphan_process` | consistency | `orphan_reaping` | the global reaper |
| `ptrace_abuse` | disclosure | `ptrace_restrict` | the gate a future `ptrace()` needs |
| `toctou_race` | trust | `atomic_check` | illustration (check-then-use gap) |
| `process_injection` | isolation | `process_isolation` | concept (needs per-process AS) |
| `process_hijacking` | integrity | `control_flow_integrity` | concept (CFI is compiler+hw) |
| `container_escape` | isolation | `namespace_isolation` | concept (no namespaces) |
| `process_dos` | liveness | `service_limits` | umbrella over slot_quota + CPU quotas |

The pairs are nested as `security/<attack>/defence/<defence>/`; see
[security/README.md](security/README.md). The last eight were added in
the restructure — some are honest illustrations/concepts because UzOS is
a single-address-space kernel (see Known limits). Each folder has a
README. Try `seclab list`, `seclab attack <id> live`
and `seclab verify all`. The lab itself lives in [`modules/security_management`](../security_management/).

## Known limits

- **One address space.** All processes share the same page directory, so
  they could read each other's memory. Per-process page directories are
  the next big step; `fork()` waits for them.
- **ELF programs run in ring 0** (as the user who started them). Only
  usertask programs (`testuser`, the cpu_management attacks) really run
  in ring 3. A ring 0 program could even write into the kernel's code;
  integrity_check (security_management) notices at the next `exec`.
