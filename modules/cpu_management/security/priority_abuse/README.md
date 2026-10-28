# Attack: priority_out_of_range

| | |
|---|---|
| **Stopped by** | [`priority_bounds`](../../defence/priority_bounds/) |
| **Breaks** | trust: numbers from a program are checked before use |
| **Severity** | high |

## What it does

`setpriority(which, who, nice)` and `sched_setscheduler(pid, policy,
param)` take **every** number from the caller. The ring 3 program
[priority_out_of_range.s](priority_out_of_range.s) runs as user 1 and
asks for six things it should not get:

| Call | Without a check |
|---|---|
| `setpriority(me, 1000)` | beyond every level **and** the aging cap: nothing could ever take the CPU back |
| `setpriority(me, -500)` | a nice value that does not exist |
| `setpriority(me, -20)` | a normal user jumps ahead of everyone |
| `setpriority(2, 19)` | the shell (kernel_main) pushed to the bottom: the OS crawls for everybody |
| `setpriority(1, -20)` | the idle task competes with real work |
| `sched_setscheduler(me, SCHED_FIFO, 99)` | real-time: runs before every normal process |

Each answer (0 = done, -1 = refused) is left in the usertask mailbox,
and the kernel prints them after the program exits.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the five calls and what each would do |
| `live` | runs the program in ring 3; all six calls should come back `refused (-1)` |

## Uses

- `usertask_start()` / `usertask_mailbox()` from process_management/core/usertask
- syscall 21 `setpriority` ([syscalls/setpriority](../../../syscalls/setpriority/))
- syscall 26 `sched_setscheduler` ([syscalls/sched_setscheduler](../../../syscalls/sched_setscheduler/))

## Try it

```
seclab attack priority_out_of_range live
audit
```
