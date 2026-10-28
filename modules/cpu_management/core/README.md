# cpu_management/core — only what the kernel itself runs, or what many folders share as state

cpu_management is built from four kinds of folders:

| Folder | Runs when | Holds |
|---|---|---|
| `commands/<name>/` | you type the command | everything that command needs; helpers copied in, several files if needed |
| `syscalls/<name>/` | a program does `int 0x80` | everything that syscall needs; helpers copied in, several files if needed |
| `security/<attack>/` (defence inside) | `seclab`, or the kernel asks a defence "is this allowed?" | the attack's program (`.s`), its defence/`<name>`/, a README |
| `core/<part>/` | **all the time**, without anyone asking | the CPU machinery and its state |

## The rule: what may live in core

A part lives in `core/` only if at least one of these is true:

1. **The kernel calls it by itself**: the timer, the scheduler,
   process creation, the reaper, another module.
2. **It holds state that several folders must share.** There is only
   ONE nice value per process and ONE tick counter. If `nice` had its
   own copy of the priority code, the scheduler would never see it.
   Copying code is fine; copying state is a bug.

Everything else lives in the folder that uses it, copied into each
folder if several use it. That is why there is no `core/launch` (each
command that starts a child has its own `run_child.c`), no
`core/thread` (`clone` and `clone3` each have their own thread code),
no `core/ksyms` (only `perf report` needs function names), and why the
perf counters live in `syscalls/perf_event_open/`. `/proc` is a
filesystem driver, so it lives in `filesystem_management/core/procfs/`.

## The parts, and who calls them

| Part | What it is | Called by the kernel (rule 1) | Shared state (rule 2) |
|---|---|---|---|
| `info/` | cpuid: vendor, model, features | kernel_main at boot | the decoded CPU info |
| `faults/` | a handler for every CPU exception | the CPU (exceptions), memory_management/vmm | the fault log and counts |
| `tsc/` | cycle counter, clock speed | kernel_main at boot, core/accounting | the measured MHz |
| `accounting/` | ticks per process, exact task clock, 60 s history | the timer (every tick), the scheduler (every switch) | CPU totals, history |
| `loadavg/` | load average | core/accounting (every tick) | the three averages |
| `priority/` | nice -> levels, aging, RT throttling, who runs next | the scheduler (every tick and switch) | throttling state |
| `policy/` | SCHED_OTHER/FIFO/RR/BATCH/IDLE | core/priority (slice length) | the one place a policy is changed |
| `affinity/` | CPU masks | core/task (a new process's mask) | the one place a mask is changed |
| `rlimit/` | resource limits | process_create, open, write, sbrk, the timer (RLIMIT_CPU) | the limit rules |
| `numa/` | nodes and memory policies | core/task (a new process's policy) | mbind ranges, read by `mbind` and `get_mempolicy` |
| `task/` | what a child inherits, what a dead process used | process_create, the reaper | usage records, read by `time`, `perf stat` |
| `perf/` | samples for `perf record` | the timer (every tick, via accounting) | the sample buffer |
| `power/` | power profiles, the idle loop | the idle task (scheduler) | the current profile |

Every file names what it uses at its top ("Uses: ...") and includes
it by its full path, e.g.

```c
#include "modules/cpu_management/core/tsc/tsc.h"         // tsc_mhz
```
