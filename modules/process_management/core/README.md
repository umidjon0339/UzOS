# process_management/core — only what the kernel itself runs, or what many folders share as state

process_management is built from four kinds of folders, the same as
[cpu_management](../../cpu_management/core/README.md):

| Folder | Runs when | Holds |
|---|---|---|
| `commands/<name>/` | you type the command | everything that command needs; helpers copied in, several files if needed |
| `syscalls/<name>/` | a program does `int 0x80` | everything that syscall needs; helpers copied in, several files if needed |
| `security/<attack>/` (defence inside) | `seclab`, or the kernel asks a defence "is this allowed?" | the attack, its defence/`<name>`/, a README each |
| `core/<part>/` | **all the time**, without anyone asking | the process machinery and its state |

## The rule: what may live in core

A part lives in `core/` only if at least one of these is true:

1. **The kernel calls it by itself**: the timer, the scheduler, an
   interrupt, process creation, the reaper, another module.
2. **It holds state that several folders must share.** There is ONE
   process table and ONE pending-signal word per process. If `kill`
   kept its own copy, the process would never see the signal.
   Copying code is fine; copying state is a bug.

Everything else lives in the folder that uses it, copied into each
folder if several use it. That is why `wait`, `waitpid` and `waitid`
each do their own polling, and why every command parses its own
arguments.

## The parts, and who calls them

| Part | What it is | Called by the kernel (rule 1) | Shared state (rule 2) |
|---|---|---|---|
| `process/` | the process table: create, kill, exit | every module that starts or ends a process | the table, the next PID |
| `scheduler/` | who runs now, the task switch | the timer (every tick), every sleep and exit | the current and idle process |
| `signal/` | send, deliver (`deliver.c`), queue values (`sigqueue.c`), alarms | every return to ring 3 (isr hook), the timer (alarms), Ctrl+C, the reaper | pending bits, handlers, the queue pool |
| `elf/` | load an ELF file and start it | `exec`, the security lab | (none: rule 1) |
| `exitlog/` | exit records of finished processes | the reaper (writes) | read by every wait syscall: each child reported once |
| `pidfd/` | pidfd nodes: a handle per process | fd_cleanup when a process ends | shared by pidfd_open, pidfd_send_signal, pidfd_getfd, waitid |
| `usertask/` | run a small program in ring 3 | `testuser`, `cpucalls`, the attacks | the two user pages, the mailbox |

Every file names what it uses at its top ("Uses: ...") and includes
it by its full path, with what it takes from it:

```c
#include "modules/process_management/core/process/process.h"   // process_get_by_pid
```
