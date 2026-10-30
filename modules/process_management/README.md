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
│   ├── exitlog/    exitlog.h/.c       exit status + usage of finished processes, until wait()
│   ├── pidfd/      pidfd.h/.c         file descriptors that name one process
│   ├── signal/     signal.h/.c        send, names, default actions, alarm countdown
│   │               deliver.c          run a handler / default action on the way to ring 3
│   │               sigqueue.c         signals that carry a value (rt_sigqueueinfo)
│   └── usertask/   usertask.h/.c      run a small .s program in ring 3 (testuser, attacks)
├── commands/                     shell commands, one folder each
│   ├── proc_commands.h/.c             the command table
│   ├── kill/ spawn/ sleep/ exec/          (ps and top: cpu_management)
│   ├── testuser/   cmd_testuser.c     run a program in ring 3 (via usertask)
│   │               user_program.s     ...these programs
│   ├── procinfo/ pstree/ pgrep/ pidof/ pwdx/   looking at processes
│   ├── pkill/ killall/                         signalling by name
│   ├── jobs/ fg/ bg/ wait/                     job control (Ctrl+Z stops the foreground job)
│   ├── nohup/ setsid/ timeout/                 run a command apart (each with its own start_child.c)
│   ├── strace/ pmap/                           debugging: syscalls, memory map
│   └── proccalls/  cmd_proccalls.c    ring 3 programs call every process syscall
│                   signals.s ... debug.s exec.s   one program per group
│                   showargs.s         a whole ELF file, byte by byte (run by exec.s)
├── syscalls/                     one folder per system call: sys_<name>.h/.c
│   ├── handler/    syscall_handler.h/.c   int 0x80 -> the right sys_*()
│   ├── exit/ sleep/
│   ├── getppid/ setpgid/ getpgid/ getpgrp/ setsid/ getsid/   identity, groups, sessions
│   ├── kill/ tkill/ tgkill/ pidfd_send_signal/               signals (signal_permission gate)
│   ├── rt_sigaction/ rt_sigprocmask/ rt_sigpending/ rt_sigsuspend/
│   │   rt_sigtimedwait/ rt_sigqueueinfo/ rt_tgsigqueueinfo/ rt_sigreturn/
│   │   sigaltstack/ pause/ alarm/                            handlers, masks, queues
│   ├── prctl/                                                name, pdeathsig, subreaper, ...
│   ├── wait/ waitpid/ waitid/ wait4/                          wait for a child (child_only)
│   ├── getrusage/ times/                                      CPU used, by me / my children
│   ├── pidfd_open/ pidfd_getfd/                               handles on a process
│   ├── nanosleep/ clock_nanosleep/ setitimer/ getitimer/      sleeping, interval timers
│   ├── seccomp/ unshare/ setns/                               sandboxes (namespaces: honest -1)
│   ├── ptrace/ process_vm_readv/ process_vm_writev/ kcmp/     debugging
│   ├── vfork/ execve/ execveat/          start a program: vfork + execve (real)
│   └── fork/                             -1: needs per-process address spaces
│                                          (getpid, sched_yield, clone: cpu_management)
└── security/                     one folder per ATTACK; its DEFENCE nested inside
    └── <attack>/   atk_<attack>.c + README + defence/<defence>/def_<defence>.h/.c
                    (39 pairs; see security/README.md)
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

The reaper frees a zombie at once, but first writes its **exit record**
(core/exitlog: pid, parent, status, CPU used). `wait4()` / `waitpid()` /
`waitid()` / `wait()` take that record, so a parent may ask any time
later, and each child is reported once. The status is Linux's: `exit(n)`
gives `n << 8`, a deadly signal `s` gives `s` (`| 0x80` if it would dump core).

- When a process ends (`process_ended()`), its children are re-parented
  to the nearest ancestor with `prctl(PR_SET_CHILD_SUBREAPER)`, else to
  PID 1, and get their `pdeathsig`; its parent gets `SIGCHLD`.
- `logout` hangs up the session (`process_hangup()`): every job gets
  SIGHUP and ends, unless it ignores SIGHUP (`nohup`; an ignored signal
  stays ignored in everything it starts) or lives in another session
  (`setsid`). Ctrl+Z sends SIGTSTP to the shell; `exec` and `fg` then
  stop their program (`T` in `ps`) and `jobs` / `fg` / `bg` take over.
- Every process belongs to a user (`process_t.uid`). A new one gets the
  uid of whoever created it (`process_caller_uid()`); the shell's uid is
  the logged-in user (user_management). `ps` and `top` (cpu_management) show the name.
- `scheduler_init()` creates two **system** processes: `idle` (PID 1),
  which runs when nobody else can, and `kernel_main` (PID 2), the code
  that was already running, which is the shell. Neither can be killed.
- A process never frees its own stack, because it is standing on it. It
  becomes a `ZOMBIE`, and the reaper frees it on a later timer tick.

## Signals

A **signal** is a one-bit message to a process (`process_t.signals`),
numbered as in Linux: 1 `SIGHUP` ... 31 `SIGSYS` (`kill -l` lists them).
What happens next depends on the process:

| Process | When | What |
|---|---|---|
| ring 3 program | on every return to ring 3 (after a syscall or an interrupt) | `deliver.c` runs its **handler**, or the **default action**: end (exit status = the signal), stop, or ignore |
| ring 0 process (shell, spawn, exec) | when it chooses | it looks at its bits (`signal_interrupted()`) |
| anyone but a system process | at once | `SIGSTOP` freezes it (state `T` in `ps`), `SIGCONT` thaws it, `SIGKILL` ends it |

```
kill(pid, SIGUSR1)            bit 10 set
  ... the program returns to ring 3 ...
deliver.c                     push a sigframe_t on its stack (registers, mask, siginfo)
                              EIP = handler, ESP = the frame
handler(10) runs, 'ret'   ->  the frame's trampoline: mov eax, 76 ; int 0x80
rt_sigreturn                  sigframe_check, then every register comes back
```

A blocked signal (`rt_sigprocmask`) waits; an ignored one is thrown away
when sent. `rt_sigqueueinfo` sends a value too: every one is queued
(`sigqueue.c`, 64 entries, 16 per user: `sigqueue_limit`).
Each process has three **interval timers** (`setitimer`): `ITIMER_REAL`
(wall clock → `SIGALRM`; `alarm()` is this one), `ITIMER_VIRTUAL` (its
ring 3 CPU time → `SIGVTALRM`) and `ITIMER_PROF` (all its CPU time →
`SIGPROF`). `signal_tick()` runs them on every timer tick, after
cpu_management's accounting has charged the tick.

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

| # | Syscall | Arguments (EBX, ECX, EDX, ESI) | Result (EAX) |
|---|---------|----------------|--------------|
| 1 | `exit`   | exit code     | never returns (exit status = code << 8) |
| 3 | `sleep`  | milliseconds  | 0 |
| 52–57 | `getppid`, `setpgid`, `getpgid`, `getpgrp`, `setsid`, `getsid` | ids | the id, or -1 |
| 53, 56 | `setpgid`, `setsid` | | 0 / the session, or -1 (session_check) |
| 58 | `kill` | pid (> 0 one, 0 my group, -1 all I may, < -1 group), signal | 0 or -1 (signal_permission) |
| 59–61 | `tkill`, `tgkill`, `pidfd_send_signal` | target, signal | 0 or -1 (signal_permission) |
| 62–64 | `wait`, `waitpid`, `waitid` (idtype, id, info, options) | child | its pid, or -1 (child_only) |
| 65 | `fork` | | -1: a copy of memory needs per-process address spaces |
| 66 | `vfork` | | the child's pid; 0 in the child, which borrows memory and stack until it execs |
| 67 | `execve` | path, argv, envp | no return on success (the new program in ring 3); -1 (arg_limits, elf_validate) |
| 68 | `execveat` | dirfd (AT_FDCWD), path, argv, envp, flags | as execve |
| 69 | `rt_sigaction` | sig, act, oldact, setsize | 0 or -1 (handler_bounds) |
| 70 | `rt_sigprocmask` | how, set, oldset, setsize | 0 or -1 |
| 71 | `rt_sigpending` | set, setsize | 0 |
| 72 | `rt_sigsuspend` | mask, setsize | -1 (EINTR) after a handler |
| 73 | `rt_sigtimedwait` | set, info, timeout, setsize | the signal, or -1 |
| 74 | `rt_sigqueueinfo` | pid, sig, info | 0 or -1 (sigqueue_limit) |
| 75 | `rt_tgsigqueueinfo` | tgid, tid, sig, info | 0 or -1 |
| 76 | `rt_sigreturn` | (the frame on the stack) | the saved EAX (sigframe_check) |
| 77 | `sigaltstack` | ss, old_ss | 0 or -1 |
| 78 | `pause` | | -1 (EINTR) after a handler |
| 79 | `alarm` | seconds | seconds left of the old alarm |
| 80 | `prctl` | option, arg2 | 0 / the value, or -1 (name_guard) |
| 81 | `wait4` | pid, &status, options, &rusage | the child's pid, 0 (WNOHANG), -1 (child_only) |
| 82 | `getrusage` | who, &rusage | 0 or -1 |
| 83 | `times` | &tms | ticks since boot |
| 84 | `pidfd_open` | pid, flags | a descriptor, or -1 (pidfd_handle) |
| 85 | `pidfd_getfd` | pidfd, fd, flags | a copy of that process's fd, or -1 (pidfd_handle) |
| 86 | `nanosleep` | &req, &rem | 0, or -1 (EINVAL: sleep_bounds; EINTR: *rem left) |
| 87 | `clock_nanosleep` | clock, flags (TIMER_ABSTIME), &req, &rem | 0, 22 (EINVAL), 4 (EINTR), as Linux |
| 88 | `setitimer` | which (REAL/VIRTUAL/PROF), &new, &old | 0 or -1 (sleep_bounds, timer_limits) |
| 89 | `getitimer` | which, &cur | 0 or -1 |
| 90 | `seccomp` | op (STRICT / FILTER / GET_ACTION_AVAIL), flags, args | 0 or -1 (no_new_privs) |
| 91 | `unshare` | flags | 0 for FS/FILES/SYSVSEM (already private), -1 for namespaces |
| 92 | `setns` | fd, nstype | -1: UzOS has no namespaces |
| 93 | `ptrace` | request (TRACEME, PEEKDATA, POKEDATA, CONT, KILL, GETREGS, ATTACH, DETACH), pid, addr, data | 0 or -1 (ptrace_restrict, vm_access_check) |
| 94–95 | `process_vm_readv`, `process_vm_writev` | pid, local iov, n, remote iov, n, flags | bytes copied, or -1 (vm_access_check) |
| 96 | `kcmp` | pid1, pid2, type, idx1, idx2 | 0 same, 1/2 different, -1 (hidepid) |

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

A process in a **seccomp** sandbox is checked before every call
(`seccomp_filter`): strict mode keeps `read`, `write`, `exit`,
`rt_sigreturn`; a filter keeps the syscalls it listed. A refused call
fails (-1) or ends the program with SIGSYS. The sandbox only ever gets
stricter and every child inherits it; a filter needs `no_new_privs`.

For calls from ring 3, every pointer argument (buffer or path) is
checked first by `user_ptr_bounds`: it must be the program's own mapped
memory, so a bad pointer fails only that call.

## Commands

`ps` and `top` moved to [cpu_management](../cpu_management/README.md#commands).

| Command | What it does | Uses |
|---|---|---|
| `kill [-SIG] <pid>...`, `kill -l` | end a process (SIGKILL, the default), or send any signal: `-TERM`, `-15`, `-s USR1`, `-STOP`, `-CONT` | `process_kill` (→ kill_guard), `signal_send` (→ signal_permission) |
| `spawn` | start a worker that beats 3 times, then exits | `process_create`, `sys_sleep`, `sys_exit` |
| `sleep <s>` | pause the shell (Ctrl+C ends it) | `sys_sleep_interruptible` |
| `exec <file> [&]` | load and run an ELF program, e.g. `exec /hello`: in the foreground (Ctrl+C stops it), or with `&` in the background; refused if the kernel's code changed (integrity_check) | `elf_load_and_run`, `sys_sleep_interruptible`, `process_kill` |
| `procinfo [pid]` | everything about one process: family, time, signals, cwd, files, prctl, kernel stack | process_t, hidepid, kptr_hide |
| `pstree [-p] [pid]` | the family tree (`ppid`) | process table |
| `pgrep [-l] [-x] [-u user] pattern` | PIDs whose name contains / is `pattern` | process table |
| `pidof name...` | PIDs of exactly that name, newest first | process table |
| `pwdx pid...` | working directory of a process | hidepid |
| `pkill [-SIG] [-x] [-u user] pattern` | signal by name (default SIGKILL) | kill_guard, signal_permission |
| `killall [-SIG] name` | signal everything called `name` | kill_guard, signal_permission |
| `jobs` | the shell's children: `[1]+ Running worker (PID 5)` | process table |
| `fg [%job]` | continue a job and wait for it; Ctrl+C ends it, Ctrl+Z stops it again | SIGCONT, exitlog |
| `bg [%job]` | a stopped job carries on in the background | SIGCONT |
| `wait [%job\|pid]...` | wait for jobs, print how each ended | exitlog |
| `nohup command` | run in the background, ignoring SIGHUP: survives `logout` | start_child.c |
| `setsid command` | run in a new session: survives `logout`, out of the shell's group | start_child.c |
| `timeout [-s SIG] s command` | end the command after `s` seconds (then SIGKILL after 1 s) | start_child.c |
| `strace command`, `strace -p\|-d pid` | print every syscall of a program (`[strace] 9 prctl(0xf, ...) = 0`); its children too | process_t.strace, ptrace_restrict |
| `pmap [pid]` | its kernel stack and the mapped user pages (shared: one address space) | vmm_walk, hidepid, kptr_hide |
| `proccalls [signals\|prctl\|wait\|session\|timers\|sandbox\|debug\|exec]` | ring 3 programs call every process syscall; each answer is checked (signals 49, prctl 20, wait 34, session 16, timers 32, sandbox 19, debug 27, exec 17: 214) | usertask, `.s` programs in its folder |
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
| `ptrace_abuse` | disclosure | `ptrace_restrict` | `ptrace` (ATTACH, TRACEME), `strace -p` |
| `toctou_race` | trust | `atomic_check` | illustration (check-then-use gap) |
| `process_injection` | isolation | `process_isolation` | concept (needs per-process AS) |
| `process_hijacking` | integrity | `control_flow_integrity` | concept (CFI is compiler+hw) |
| `container_escape` | isolation | `namespace_isolation` | concept (no namespaces) |
| `process_dos` | liveness | `service_limits` | umbrella over slot_quota + CPU quotas |
| `signal_flood` | resource | `sigqueue_limit` | `sigqueue_add` (rt_sigqueueinfo) |
| `sigreturn_forgery` | privilege | `sigframe_check` | `rt_sigreturn` |
| `handler_hijack` | integrity | `handler_bounds` | `rt_sigaction` |
| `process_masquerade` | trust | `name_guard` | `prctl(PR_SET_NAME)` |
| `proc_info_leak` | disclosure | `hidepid` | `procinfo`, `pwdx`, `/proc/<pid>/status` |
| `kernel_pointer_leak` | disclosure | `kptr_hide` | `procinfo` |
| `pid_reuse_race` | identity | `pidfd_handle` | `pidfd_open`, `pidfd_getfd` |
| `wait_steal` | privilege | `child_only` | `wait4`, `waitpid`, `waitid` |
| `use_after_exit` | consistency | `pcb_generation` | core/pidfd (every pidfd use) |
| `pgid_hijack` | isolation | `session_check` | `setpgid`, `setsid` |
| `sleep_overflow` | liveness | `sleep_bounds` | `sleep`, `nanosleep`, `clock_nanosleep`, `setitimer`, `rt_sigtimedwait` |
| `timer_flood` | resource | `timer_limits` | `setitimer` |
| `syscall_abuse` | privilege | `seccomp_filter` | the syscall handler, before every call |
| `no_new_privs_bypass` | privilege | `no_new_privs` | `seccomp`, `prctl`, `process_create` |
| `memory_peek` | disclosure | `vm_access_check` | `process_vm_readv`/`writev`, `ptrace` PEEK/POKE |
| `argv_overflow` | resource | `arg_limits` | `execve` |
| `env_injection` | trust | `env_sanitize` | `execve` |
| `fd_leak_on_exec` | disclosure | `close_on_exec` | `execve` |
| `thread_exhaustion` | resource | `thread_quota` | `clone`, `clone3` (cpu_management) |
| `stack_clash` | isolation | `guard_gap` | `execve` |
| `core_dump_leak` | disclosure | `dumpable_flag` | `setuid` (user_management) |
| `priority_inversion` | liveness | `priority_inheritance` | concept (no shared locks / futex) |
| `tty_hijack` | integrity | `tty_guard` | concept (no input injection) |

The pairs are nested as `security/<attack>/defence/<defence>/`; see
[security/README.md](security/README.md). Most attacks are real: many
run a ring 3 program (`.s` in the attack's folder) against the live
kernel; a few are honest concepts because UzOS has no namespaces, no
shared locks and no terminal injection. Try `seclab list`,
`seclab attack <id> live` and `seclab verify all`. The lab itself lives
in [`modules/security_management`](../security_management/).

## Starting a program: vfork + execve

```
vfork()        the child borrows my memory and my stack; I wait
  child: execve("/home/guest/showargs", argv, envp)
           arg_limits, env_sanitize: copy argv / envp into the kernel
           elf_validate: the file
           load its segments, build its stack (argc, argv, envp) in the
           process's own 64 KiB area at 0xB0000000 + slot * 64 KiB
           close_on_exec, handlers back to SIG_DFL
           -> ring 3 at e_entry: the same pid, a new program
I go on: vfork() returns the child's pid; wait4() later gives its status
```

`execve()` always starts the new program in **ring 3**: a new program
never gets more privilege than a user program has.

## Known limits

- **One address space.** All processes share the same page directory, so
  they could read each other's memory. Per-process page directories are
  the next big step; `fork()` (a COPY of memory) waits for them and
  returns -1 until then. `vfork()` + `execve()` work without a copy.
- **The `exec` COMMAND runs an ELF program in ring 0** (as the user who
  started it: `hello` writes to the screen directly). A ring 0 program
  could even write into the kernel's code; integrity_check
  (security_management) notices at the next `exec`. The `execve()`
  SYSCALL always starts its program in ring 3.
