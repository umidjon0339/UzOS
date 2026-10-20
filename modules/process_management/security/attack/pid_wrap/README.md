# Attack: pid_wrap

| | |
|---|---|
| **Stopped by** | [`pid_bounds`](../../defence/pid_bounds/) |
| **Breaks** | identity: one PID = one process |
| **Severity** | high |

## What it does

`next_pid` counts up: 1, 2, 3, ... A 32-bit counter wraps after
`0xFFFFFFFF` back to 0, then 1, and PID 1 (`idle`) is still alive. Two
processes would then share a PID, and `kill 1` or `process_get_by_pid(1)`
would hit whichever one comes first in the table.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | highest live PID, and how many live PIDs a wrap would reuse |
| `live` | 4 billion creations is too many, so it asks the guard about the edge PIDs: `1`, `0xFFFFFEFF`, `0xFFFFFF00`, `0xFFFFFFFF`, `0` |

## Uses

`def_pid_bounds_check()` from security/defence/pid_bounds, the process table.

## Try it

```
seclab attack pid_wrap live
```
