# Attack: priority_out_of_range

| | |
|---|---|
| **Stopped by** | [`priority_bounds`](../../defence/priority_bounds/) |
| **Breaks** | trust: numbers from a program are checked before use |
| **Severity** | high |

## What it does

`setpriority(pid, level)` takes **both** numbers from the caller. The
ring 3 program [priority_out_of_range.s](priority_out_of_range.s) runs
as user 1 and asks for five things it should not get:

| Call | Without a check |
|---|---|
| `setpriority(me, 1000)` | above every level **and** above the aging cap (7): nothing could ever take the CPU back |
| `setpriority(me, -5)` | below the idle task; read as unsigned it is 4 billion, the very top |
| `setpriority(me, 7)` | a normal user jumps ahead of everyone |
| `setpriority(2, 1)` | the shell (kernel_main) pushed to the bottom: the OS crawls for everybody |
| `setpriority(1, 7)` | the idle task competes with real work |

Each answer (0 = done, -1 = refused) is left in the usertask mailbox,
and the kernel prints them after the program exits.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the five calls and what each would do |
| `live` | runs the program in ring 3; every call should come back `refused (-1)` |

## Uses

- `usertask_start()` / `usertask_mailbox()` from process_management/core/usertask
- syscall 21 `setpriority` ([syscalls/setpriority](../../../syscalls/setpriority/))

## Try it

```
seclab attack priority_out_of_range live
audit
```
