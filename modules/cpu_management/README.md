# cpu_management — the CPU itself: who it is, where its time goes, who gets it next

process_management decides **what a process is** and knows **how** to
switch the CPU from one process to another. This module manages the
**CPU**:

| Question | Answered by |
|---|---|
| What CPU is this, what can it do? | `core/info`: cpuid features, control registers |
| How fast is it? | `core/tsc`: cycle counter, measured against the PIT |
| What if it refuses an instruction? | `core/faults`: a ring 3 fault kills that process, a ring 0 fault halts |
| Where did its time go? | `core/accounting`: every tick charged to a process, ring 0 or ring 3 |
| Who gets it next, and for how long? | `core/priority`: levels 1–7, 50 ms turns, aging |

```
cpu_management/
├── core/                         the machinery, one folder per part
│   ├── info/        cpu_info.h/.c      cpuid leaf 1: family/model/stepping, features
│   ├── faults/      cpu_faults.h/.c    every CPU exception -> kill process or halt
│   ├── tsc/         tsc.h/.c           rdtsc, clock speed in MHz
│   ├── accounting/  cpu_account.h/.c   CPU time per process, load per second
│   └── priority/    priority.h/.c      scheduling policy: levels, slices, aging
├── commands/                     shell commands, one folder each
│   ├── cpu_commands.h/.c              the command table (group "CPU")
│   └── cpuinfo/ faults/ cpustat/ nice/
├── syscalls/                     one folder per system call
│   └── cputime/ setpriority/ getpriority/
└── security/                     attack/defence pairs (see security_lab)
    ├── attack/   user_exception/ tsc_timing/ priority_out_of_range/ cpu_hog/
    │             (each with its ring 3 program as a .s file, + README)
    └── defence/  fault_isolation/ tsc_restrict/ priority_bounds/ aging/
```

What it uses from other folders:

| From | What |
|---|---|
| `arch/x86/cpu/cpu.h` | `cpu_rdtsc()`, `cpu_read_cr0/cr4()`, `cpu_write_cr4()`, `CR0_*` / `CR4_*` bits |
| `arch/x86/cpuid/` | `cpuid_leaf()`, vendor and brand string |
| `arch/x86/isr/` | `register_interrupt_handler()`, `isr_panic()`, `isr_exception_name()` |
| `process_management/core/usertask/` | run a small `.s` program in ring 3 (the attacks use it) |
| `device_management/timer/` | calls `cpu_account_tick()` on every tick |
| `process_management/core/scheduler/` | calls `priority_tick()` / `priority_pick_next()` |

`arch/x86` only runs instructions. Everything that **decides** something
lives here.

## Boot

`kernel_main()` adds three steps:

```
[  OK  ] CPU              GenuineIntel, family 6 model 6, 15 features   cpu_info_init()
[  OK  ] CPU exceptions   a ring 3 fault kills only that process        cpu_faults_init()
   ...
[  OK  ] Interrupts       on (sti)
[  OK  ] CPU clock        2686 MHz; rdtsc is kernel-only (CR4.TSD)      tsc_init()
```

The clock comes after `sti` because measuring it needs the timer to tick.

## One timer tick

```
PIT fires (IRQ 0, 100 times a second)
  timer_callback(regs)                                device_management/timer
    cpu_account_tick(regs)        core/accounting:    charge the tick to the
                                                      running process; ring 3
                                                      or ring 0? (saved CS)
    scheduler_tick()                                  process_management
      wake sleepers, reap zombies
      priority_tick(current)      core/priority:      slice - 1; waiting ones age
        turn over, or someone better READY?
          yes -> scheduler_yield()
                   priority_pick_next()  core/priority: highest effective level,
                                                        equal levels take turns
                   switch_task()                      process_management
```

## Priorities

```
level 7  ███  runs first                     only root may give it
level 4  ███  every new process starts here
level 1  ███  runs when nothing above is READY
level 0  ███  the idle task only
```

- The READY process with the highest **effective** level runs.
- Equal levels take turns (round robin).
- A turn lasts `CPU_SLICE_TICKS` = 5 ticks = 50 ms.
- A better process that becomes READY takes the CPU at once.
- **Aging:** effective = base + waited ticks / 10. Nobody waits forever.

A process that waits for something must **sleep**, not spin. The shell
sleeps 10 ms between looks at the keyboard. Otherwise it would always
be READY at level 4 and nothing at levels 1–3 would get the CPU.

## One CPU exception

```
ring 3 program: div ecx (ecx = 0)
  CPU raises #0 -> isr0 stub -> isr_handler -> fault_handler(regs)   core/faults
    fault_isolation: saved CS ring 3, not NMI/double fault/machine check?
      yes: cpu_fault_record()    "[CPU FAULT] #0 Division By Zero: PID 7 (div0) killed"
           process_exit()        the reaper frees it on a later tick
      no:  isr_panic()           registers + "System Halted!"
```

Page faults (#14) keep their handler in memory_management/core/vmm. It
uses the same rule and the same record.

## System calls

| # | Syscall | Arguments | Result |
|---|---|---|---|
| 20 | `cputime` | EBX = pid (0 = me) | CPU milliseconds used / -1 |
| 21 | `setpriority` | EBX = pid (0 = me), ECX = level | 0 / -1 (priority_bounds) |
| 22 | `getpriority` | EBX = pid (0 = me) | base level 0–7 / -1 |

## Commands

| Command | What it does | Uses |
|---|---|---|
| `cpuinfo` | vendor, name, signature, clock, features, CR0/CR4 switches | core/info, core/tsc |
| `cpuinfo flags` | every feature with its meaning | core/info |
| `faults` | how many processes each exception killed; the newest faults with EIP and instruction | core/faults |
| `faults list` | the 20 CPU exceptions and what they mean | core/faults |
| `cpustat` | load of the last second (ring 3 / ring 0 / idle), CPU time per process | core/accounting |
| `nice` | every process: base level, effective level, ticks waited | core/priority |
| `nice <pid> [level]` | show or set one process's level | `priority_set` (→ priority_bounds) |

`ps` and `top` (process_management) also show `PRI` and `CPU%`.

## Security: attack ↔ defence

| Attack | Breaks | Defence | Called from |
|---|---|---|---|
| `user_exception` | isolation | `fault_isolation` | every exception handler (core/faults, vmm) |
| `tsc_timing` | disclosure | `tsc_restrict` | `tsc_init` (CR4.TSD on), `fault_handler` (report) |
| `priority_out_of_range` | trust | `priority_bounds` | `priority_set` |
| `cpu_hog` | liveness | `aging` | `priority_effective`, `priority_pick_next` |

## Try it

```
cpuinfo                                  # what the CPU is, TSD on
seclab attack user_exception live        # five ring 3 crashes, the OS lives
faults                                   # ...and here they are
seclab attack tsc_timing                 # the timing gap, measured in ring 0
seclab attack tsc_timing live            # ring 3 rdtsc is killed
spawn
nice                                     # priorities and aging
nice 4 2                                 # (use the PID spawn printed)
seclab attack cpu_hog live               # starvation, rescued by aging
cpustat                                  # where the time went
top                                      # live: CPU bar, PRI, CPU%
```
