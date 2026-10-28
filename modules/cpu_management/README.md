# cpu_management — the CPU itself: who it is, where its time goes, who gets it next

process_management decides **what a process is** and knows **how** to
switch the CPU from one process to another. This module manages the
**CPU**:

| Question | Answered by |
|---|---|
| What CPU is this, what can it do? | `core/info`: cpuid features, control registers |
| How fast is it? | `core/tsc`: cycle counter, measured against the PIT |
| What if it refuses an instruction? | `core/faults`: a ring 3 fault kills that process, a ring 0 fault halts |
| Where did its time go? | `core/accounting`: every tick charged to a process; every task switch timed exactly |
| How busy has it been lately? | `core/loadavg`: the 1/5/15-minute load average |
| Who gets it next, and for how long? | `core/priority` (nice, levels, aging) and `core/policy` (SCHED_OTHER, FIFO, RR, BATCH, IDLE) |
| Which CPUs may a process use? | `core/affinity`: CPU masks (UzOS runs on one CPU) |
| How much may a process use? | `core/rlimit`: resource limits (CPU seconds, files, ...) |
| Where should its memory come from? | `core/numa`: nodes and memory policies (one node) |
| How do threads start and end? | `syscalls/clone`, `syscalls/clone3`, `syscalls/exit_group` |
| What does a child inherit, what did it cost? | `core/task`: birth and death of a process, CPU-wise |
| What happened where, exactly? | `core/perf` (samples), `syscalls/perf_event_open` (counters), `commands/perf_report` (function names) |
| Fast or frugal? | `core/power`: power profiles and the idle state |

## The folders

cpu_management has four kinds of folders and nothing else:

| Folder | Runs when |
|---|---|
| `commands/<name>/` | you type the command |
| `syscalls/<name>/` | a program does `int 0x80` |
| `security/<attack>/` (defence inside it) | `seclab` runs it, or the kernel asks a defence |
| `core/<part>/` | all the time: the timer, the scheduler, process creation call it |

**Each command, syscall, attack and defence folder holds everything it
needs.** A helper that several of them use is COPIED into each folder
(a folder may have several files), so every folder can be read on its
own. Only real CPU machinery lives in `core/`: parts the kernel itself
calls, or state that must exist once (one nice value per process, one
tick counter). The rule and the list of who calls each core part:
[core/README.md](core/README.md).

Every `#include` names the **full path** of the file it uses, with what
it takes from it (the Makefile puts the project root on the include path):

```c
#include "modules/cpu_management/core/tsc/tsc.h"         // tsc_mhz
```

```
cpu_management/
├── core/                         the machinery (see core/README.md)
│   ├── info/        cpu_info.h/.c      cpuid: family/model/stepping, features
│   ├── faults/      cpu_faults.h/.c    every CPU exception -> kill process or halt
│   ├── tsc/         tsc.h/.c           rdtsc, clock speed in MHz
│   ├── accounting/  cpu_account.h/.c   ticks per process, exact task clock, 60 s history
│   ├── loadavg/     loadavg.h/.c       load average (1, 5, 15 minutes)
│   ├── priority/    priority.h/.c      nice -> levels, slices, aging, RT throttling
│   ├── policy/      policy.h/.c        SCHED_OTHER/FIFO/RR/BATCH/IDLE
│   ├── affinity/    affinity.h/.c      CPU masks and lists ("0-3")
│   ├── rlimit/      rlimit.h/.c        resource limits and where they are enforced
│   ├── numa/        numa.h/.c          NUMA nodes, memory policies, mbind ranges
│   ├── task/        cpu_task.h/.c      what a child inherits; usage of finished ones
│   ├── perf/        perf.h/.c          the sample buffer of perf record, perf.data format
│   └── power/       power.h/.c         power profiles, the idle loop
├── commands/                     one folder per command: cmd_<name>.h/.c (+ its own helpers)
│   ├── cpu_commands.h/.c          the table that adds them all to the shell
│   ├── nice/ chrt/ taskset/ numactl/ time/ perf_stat/ perf_record/
│   │                              + run_child.h/.c: start the command as a child
│   ├── perf_report/               + ksyms.h/.c, ksyms_gen.sh: kernel function names
│   ├── cpucalls/                  + cpucalls.s: the ring 3 test program
│   └── lscpu/ nproc/ top/ htop/ btop/ mpstat/ vmstat/ pidstat/ sar/ ps/
│       uptime/ renice/ perf/ cpupower/ turbostat/ systemd_cgtop/
│       systemctl/ powerprofilesctl/ ulimit/ getconf/ cpuinfo/ faults/ cpustat/
├── syscalls/                     one folder per system call: sys_<name>.h/.c
│   ├── clone/                     + clone_enter.s: the new thread enters ring 3
│   ├── clone3/                    + clone3_enter.s (its own copy)
│   ├── perf_event_open/           + perf_counter.h/.c: counters behind fds
│   └── (the other 27, see the table below)
└── security/                     one folder per ATTACK; its DEFENCE lives inside it,
    │                             at security/<attack>/defence/<defence>/ (+ a README
    │                             each, and the ring 3 .s program where one is used)
    ├── starvation/aging   priority_abuse/priority_bounds   side_channel/tsc_restrict
    ├── user_exception/fault_isolation
    ├── cpu_exhaustion/cpu_time_limit     infinite_loop/cpu_watchdog         (resource)
    ├── resource_exhaustion/resource_quota  fork_bomb/process_quota
    ├── denial_of_service/fair_scheduling  scheduling_manipulation/policy_restriction
    ├── realtime_scheduling_abuse/rt_throttling  privilege_escalation/permission_check
    ├── spectre/user_kernel_isolation  meltdown/page_separation         (illustrations)
    ├── cache_timing/coarse_timer      branch_prediction/speculation_barrier
    └── distributed_denial_of_service/rate_limiting  rowhammer/ecc_and_refresh  (concepts)
        return_oriented_programming/wx_and_aslr  code_injection/nx_pages
        hypervisor_escape/vm_isolation
```

The CPU fork bomb's seclab id is **`cpu_fork_bomb`** (`fork_bomb` already names
the process-table bomb in process_management). Illustrations show a real
microarchitecture attack's boundary without a working exploit; concepts are
documented where UzOS cannot reproduce them (no network, emulated RAM, no NX
bit, or because UzOS is itself the guest). See [security/README.md](security/README.md).

`/proc` (`cat /proc/stat`, ...) is a filesystem driver, so it lives in
[filesystem_management/core/procfs](../filesystem_management/core/procfs/);
the numbers it shows come from this module's core.

What it uses from other folders:

| From | What |
|---|---|
| `arch/x86/cpu/cpu.h` | `cpu_rdtsc()`, `cpu_read_cr0/cr4()`, `cpu_irq_save()` |
| `arch/x86/cpuid/` | `cpuid_leaf()`, vendor and brand string |
| `arch/x86/isr/` | `registers_t`, `register_interrupt_handler()`, `isr_irq_count()` |
| `process_management/core/` | the process table, the scheduler, signals, usertask |
| `device_management/` | timer ticks, RTC, keyboard, the screen |
| `memory_management/core/` | `pmm_*` (RAM), `vmm_walk()` (is a page mapped?), `kmalloc` |
| `filesystem_management/core/vfs` | VFS nodes: perf counters are files |
| `io_management/` | `fd_alloc()`, `open/read/write/close` |
| `security_management/core/` | `caps_has()`, `audit_log()`, the registry |
| `shell/core/shell` | `shell_run_command()`, `shell_find()` (for each `run_child.c`) |

And what other modules call here:

| Who | Calls | Why |
|---|---|---|
| `device_management/timer` | `cpu_account_tick()` | charge the tick, RLIMIT_CPU, perf samples, load average |
| `process_management/core/scheduler` | `priority_*()`, `cpu_account_switch()`, `power_idle()` | who runs next; time the switch; what idle does |
| `process_management/core/process` | `cpu_task_fork()`, `rlimit_nproc_ok()` | a new process inherits nice/policy/limits |
| `process_management/.../reaper` | `cpu_task_exit()` | a finished process's usage goes to its parent |
| `io_management` open / write | `rlimit_nofile()`, `rlimit_fsize_ok()` | RLIMIT_NOFILE, RLIMIT_FSIZE |
| `memory_management` sbrk | `rlimit_data_ok()` | RLIMIT_DATA |
| `filesystem_management/core/procfs` | `cpu_account_*`, `affinity_*`, `numa_*`, `cpu_info()` | the numbers in /proc |

`arch/x86` only runs instructions. Everything that **decides** something
about the CPU lives here.

## Boot

```
[  OK  ] CPU              GenuineIntel, family 6 model 6, 15 features   cpu_info_init()
[  OK  ] CPU exceptions   a ring 3 fault kills only that process        cpu_faults_init()
   ...
[  OK  ] /proc            cpuinfo, stat, <pid>/status                   procfs_init()
   ...
[  OK  ] Interrupts       on (sti)
[  OK  ] CPU clock        2686 MHz; rdtsc is kernel-only (CR4.TSD)      tsc_init()
```

## One timer tick

```
PIT fires (IRQ 0, 100 times a second)
  timer_callback(regs)                                 device_management/timer
    cpu_account_tick(regs)        core/accounting:     charge the tick: user / nice /
                                                       system / idle (saved CS)
      rlimit_check_cpu(p)         core/rlimit:         over RLIMIT_CPU? -> zombie
      perf_sample(regs, p)        core/perf:           perf record running? note EIP
      loadavg_tick()              core/loadavg:        every 5 s: blend the load
    scheduler_tick()                                   process_management
      wake sleepers, reap zombies (reaper -> cpu_task_exit)
      priority_tick(current)      core/priority:       RT throttling, slice - 1,
                                                       someone better READY?
        yes -> scheduler_yield()
                 priority_pick_next()  core/priority:  highest rank, round robin
                 cpu_account_switch()  core/accounting: exact cycles, ctx switches
                 switch_task()                          process_management
```

## Priorities: nice, levels, policies

Users speak **nice** (-20 = greedy ... 19 = nice), as in Unix. The
scheduler turns it into one of seven levels; **aging** lifts a waiting
process one level per 100 ms so nobody starves:

```
nice -20 .. -16   level 7  ███  runs first          (only root may lower nice)
nice  -3 ..  1    level 4  ███  nice 0: every new process
nice  14 .. 19    level 1  ███  runs when nothing above is READY
                  level 0  ███  the idle task only
```

The **policy** decides which kind of rule applies (core/policy):

| Policy | Rank (bigger runs first) | A turn lasts | `ps` CLS |
|---|---|---|---|
| `SCHED_FIFO` 1..99 | 100 + priority | until it sleeps | FF |
| `SCHED_RR` 1..99 | 100 + priority | 100 ms | RR |
| `SCHED_OTHER` | level × 10 (with aging) | 50 ms | TS |
| `SCHED_BATCH` | level × 10, never preempts | 50 ms | B |
| `SCHED_IDLE` | 5 (above the idle task only) | 50 ms | IDL |

Real-time processes beat every normal one, but get at most **95% of each
second** (RT throttling, as Linux's `sched_rt_runtime_us`): a runaway
FIFO loop cannot freeze the shell. `chrt -f 50 seclab attack starvation live`
shows the message `[sched] RT throttling activated`.

A new process **inherits** nice, policy, CPU mask, limits and memory
policy from its creator (core/task), like `fork()`. That is why
`nice -n 10 spawn` gives the worker nice 10, and `ulimit -t 2` limits
every command started after it.

## Running a command with other settings

`nice`, `chrt`, `taskset`, `numactl`, `time` and `perf stat/record` run
their command as a **new process** (each with its own `run_child.c`),
change ITS settings before it may run, and wait. The shell itself is never changed. The
finished process's usage record (core/task) is how `time` and
`perf stat` learn what it cost, like `wait4()` in Unix. Ctrl+C is passed
on to the child.

## System calls

`int 0x80`, EAX = number, arguments in EBX, ECX, EDX, ESI, EDI, EBP.
Errors return -1. Pointers from ring 3 are checked by `user_ptr_bounds`
(syscall handler) before the call. `cpucalls` runs a ring 3 program that
calls every one of them and checks each answer.

| # | Syscall | Arguments | Result |
|---|---|---|---|
| 2 | `getpid` | – | process (thread group) id |
| 42 | `gettid` | – | this thread's id |
| 4 | `sched_yield` | – | 0 |
| 20 | `cputime` | pid (0 = me) | CPU milliseconds |
| 21 | `setpriority` | which, who, nice | 0 / -1 (priority_bounds) |
| 22 | `getpriority` | which, who | 20 - nice (1..40) |
| 26 | `sched_setscheduler` | pid, policy, &param | 0 / -1 |
| 27 | `sched_getscheduler` | pid | policy |
| 28 | `sched_setparam` | pid, &param | 0 / -1 |
| 29 | `sched_getparam` | pid, &param | 0 / -1 |
| 30 | `sched_get_priority_min` | policy | 1 (FIFO/RR), 0 |
| 31 | `sched_get_priority_max` | policy | 99 (FIFO/RR), 0 |
| 32 | `sched_rr_get_interval` | pid, &timespec | 0; RR 100 ms, OTHER 50 ms, FIFO 0 |
| 33 | `sched_setaffinity` | pid, size, &mask | 0 / -1 (no such CPU) |
| 34 | `sched_getaffinity` | pid, size, &mask | bytes written (4) |
| 35 | `sched_setattr` | pid, &attr, flags | 0 / -1 |
| 36 | `sched_getattr` | pid, &attr, size, flags | 0 / -1 |
| 37 | `getrlimit` | resource, &rlimit | 0 / -1 |
| 38 | `setrlimit` | resource, &rlimit | 0 / -1 (raise needs CAP_RESOURCE) |
| 39 | `prlimit64` | pid, resource, &new, &old | 0 / -1 |
| 40 | `getcpu` | &cpu, &node | 0 (CPU 0, node 0) |
| 41 | `perf_event_open` | &attr, pid, cpu, group_fd, flags | fd: read 8 bytes = count |
| 43 | `clone` | flags, stack, &ptid, tls, &ctid | child id / 0 in the child |
| 44 | `clone3` | &clone_args, size | child id / 0 in the child |
| 45 | `move_pages` | pid, count, pages, nodes, status, flags | 0; status per page |
| 46 | `mbind` | addr, len, mode, &nodes, maxnode, flags | 0 / -1 |
| 47 | `set_mempolicy` | mode, &nodes, maxnode | 0 / -1 |
| 48 | `get_mempolicy` | &mode, &nodes, maxnode, addr, flags | 0 / -1 |
| 49 | `migrate_pages` | pid, maxnode, &old, &new | pages not moved (0) |
| 50 | `membarrier` | cmd, flags, cpu | QUERY: supported set; 0 / -1 |
| 51 | `exit_group` | status | never returns: ends every thread |

What is honest about a one-CPU, one-node PC: affinity masks must contain
CPU 0; memory policies may only name node 0; `clone()` needs `CLONE_VM`
(there is no `fork()` yet: one address space); hardware counters other
than the TSC are "not supported" (no PMU driver). Everything else is
real.

## Commands

| Command | What it does | Uses |
|---|---|---|
| `lscpu [-e]` | architecture, model, flags, hypervisor, NUMA | core/info, tsc, affinity, numa |
| `nproc [--all]` | CPUs the caller may use | core/affinity |
| `top [-b] [-d s] [-n N] [-p PID]` | live CPU, memory, processes; keys q P N T 1 | accounting, loadavg |
| `htop` | interactive: arrows select, `[` `]` renice, `k` kill | priority, kill_guard |
| `btop` | boxes: 60 s CPU graph, memory, processes | accounting history |
| `ps [-e] [-f] [-L] [aux] [-o ...] [--sort=...]` | process snapshot; `ps -o help` lists the columns | process table |
| `uptime [-p\|-s]` | time, up time, users, load average | core/loadavg |
| `mpstat [-P ALL] [i [n]]` | %usr %nice %sys %idle per CPU | accounting totals |
| `vmstat [i [n]]` | r b, free memory, in/s, cs/s, us sy id | accounting, pmm |
| `pidstat [-w] [-p PID] [i [n]]` | %usr %system %wait %CPU per process; `-w` switches | process counters |
| `sar [-u\|-q\|-w] [i [n]]` | live, or the kernel's last-minute history | accounting history |
| `nice [-n N] command` | run with a niceness (`nice` alone: the shell's) | run_child.c (own), core/priority |
| `renice N -p PID \| -u USER` | change niceness of running processes | `setpriority` |
| `chrt -p PID`, `chrt -f 10 cmd`, `chrt -m` | show / set policy and RT priority | run_child.c (own), core/policy |
| `taskset -p PID`, `taskset -c 0 cmd` | show / set CPU affinity | run_child.c (own), core/affinity |
| `numactl --hardware \| --show \| --membind=0 cmd` | NUMA nodes and placement | run_child.c (own), core/numa |
| `time command` | real / user / sys | run_child.c (own), core/task |
| `perf stat command` | task-clock, context switches, cycles | run_child.c (own), core/task |
| `perf record [-o f] command` | sample EIPs into `perf.data` | run_child.c (own), core/perf, files |
| `perf report [-i f] [--sort comm]` | overhead per function | ksyms.c (own), core/perf (file format) |
| `perf list` | the events UzOS can count | its own table |
| `cpupower frequency-info \| idle-info` | clock (measured) and idle states | tsc, power |
| `turbostat [-i s] [-n N]` | Avg_MHz, Busy%, IRQ, POLL%/C1% | accounting, power |
| `systemd-cgtop [-n N]` | CPU and memory per slice (system / user-UID) | process table |
| `systemctl [status\|show UNIT]` | services = system processes, their settings | process table, rlimit |
| `powerprofilesctl [list\|get\|set NAME]` | performance / balanced / power-saver | core/power |
| `ulimit [-SH] [-a \| -t N ...]` | the shell's resource limits | core/rlimit |
| `getconf NAME \| -a` | `_NPROCESSORS_ONLN`, `PAGESIZE`, `CLK_TCK`, ... | many |
| `cat /proc/cpuinfo`, `/proc/stat`, `/proc/PID/status` | live kernel files | filesystem_management/core/procfs |
| `cpuinfo [flags]`, `faults [list]`, `cpustat` | UzOS's own views of the CPU | core/info, faults, accounting |
| `cpucalls` | ring 3 test of every syscall above | usertask, cpucalls.s |

## Security: attack ↔ defence

21 attack/defence pairs live under `security/<attack>/`; the full list,
grouped into real demos / illustrations / concepts, is in
[security/README.md](security/README.md). The four with live kernel
hooks (the kernel itself calls the defence) are:

| Attack | Breaks | Defence | Called from |
|---|---|---|---|
| `user_exception` | isolation | `fault_isolation` | every exception handler (core/faults, vmm) |
| `side_channel` | disclosure | `tsc_restrict` | `tsc_init` (CR4.TSD on), `fault_handler` (report) |
| `priority_abuse` | trust | `priority_bounds` | `priority_set_nice`, `policy_set` |
| `starvation` | liveness | `aging` | `priority_effective`, `priority_pick_next` |

## Try it

```
lscpu                                    # the CPU, Linux-style
cpucalls                                 # every CPU syscall from ring 3: 63 of 63
nice -n 5 spawn                          # the worker inherits nice 5
ps -eo pid,ppid,comm,ni,pri,cls          # ...and here it is
chrt -b 0 ps -o pid,comm,cls             # ps itself runs as SCHED_BATCH
taskset -c 0 ps -o pid,comm,psr
top                                      # q quits
htop                                     # arrows, [ ], k, q
perf record seclab attack starvation live   # then:
perf report                              # where the time went, by function
time seclab attack starvation live
chrt -f 50 seclab attack starvation live    # RT throttling saves the shell
ulimit -t 1                              # then the hog is killed by RLIMIT_CPU:
seclab attack starvation live
cat /proc/stat
cat /proc/self/status
vmstat 1 3
sar
```
