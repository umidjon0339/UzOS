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
│   └── usertask/   usertask.h/.c      run a small .s program in ring 3 (testuser, attacks)
├── commands/                     shell commands, one folder each
│   ├── proc_commands.h/.c             the command table
│   ├── ps/ top/ kill/ spawn/ sleep/ exec/
│   └── testuser/   cmd_testuser.c     run a program in ring 3 (via usertask)
│                   user_program.s     ...these programs
├── syscalls/                     one folder per system call
│   ├── handler/    syscall_handler.h/.c   int 0x80 -> the right sys_*()
│   └── exit/ getpid/ sleep/ yield/        sys_<name>.h/.c
└── security/                     attack/defence pairs (see security_lab)
    ├── attack/<id>/   atk_<id>.c + README.md    how a guarantee breaks
    └── defence/<id>/  def_<id>.h/.c + README.md  how the kernel stops it
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

- `scheduler_init()` creates two **system** processes: `idle` (PID 1),
  which runs when nobody else can, and `kernel_main` (PID 2), the code
  that was already running, which is the shell. Neither can be killed.
- A process never frees its own stack, because it is standing on it. It
  becomes a `ZOMBIE`, and the reaper frees it on a later timer tick.

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
syscalls/getpid:   sys_getpid()  ->  regs->eax = PID
                  |
iret: back in the program, EAX = its PID
```

| # | Syscall | Argument (EBX) | Result (EAX) |
|---|---------|----------------|--------------|
| 1 | `exit`   | none          | never returns |
| 2 | `getpid` | none          | PID |
| 3 | `sleep`  | milliseconds  | 0 |
| 4 | `yield`  | none          | 0 |

The other numbers live with their own module, and the same handler's
table routes them all:

| # | Module |
|---|---|
| 5–7 | `sbrk`, `mmap`, `munmap`: [memory_management](../memory_management/syscalls/) |
| 8 | `uptime`: [device_management](../device_management/syscalls/) |
| 9–13 | `mkdir`, `unlink`, `chdir`, `getcwd`, `rename`: [filesystem_management](../filesystem_management/syscalls/) |
| 14–18 | `open`, `read`, `write`, `close`, `seek`: [io_management](../io_management/syscalls/) |
| 19 | `time`: [device_management](../device_management/syscalls/) |
| 20–22 | `cputime`, `setpriority`, `getpriority`: [cpu_management](../cpu_management/syscalls/) |

For calls from ring 3, every pointer argument (buffer or path) is
checked first by `user_ptr_bounds`: it must be the program's own mapped
memory, so a bad pointer fails only that call.

## Commands

| Command | What it does | Uses |
|---|---|---|
| `ps` | list processes, with priority (PRI) and CPU share (CPU%) | process table |
| `top` | full-screen live view: processes, memory and CPU load, every second; `q` quits and the old screen comes back | `vga_fullscreen`, `sys_sleep`, keyboard, `safe_text` |
| `kill <pid>` | stop a process | `process_kill` (→ kill_guard) |
| `spawn` | start a worker that beats 3 times, then exits | `process_create`, `sys_sleep`, `sys_exit` |
| `sleep <s>` | pause the shell | `sys_sleep` |
| `exec <file>` | load and run an ELF program, e.g. `exec hello` | `elf_load_and_run` |
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

Each folder has a README. Try `seclab list`, `seclab attack <id> live`
and `seclab verify all`. The lab itself lives in [`modules/security_lab`](../security_lab/).

## Known limits

- **One address space.** All processes share the same page directory, so
  they could read each other's memory. Per-process page directories are
  the next big step; `fork()` waits for them.
- **ELF programs run in ring 0** (with uid 1). Only usertask programs
  (`testuser`, the cpu_management attacks) really run in ring 3.
