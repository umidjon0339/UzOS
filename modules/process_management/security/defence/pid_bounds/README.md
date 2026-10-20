# Defence: pid_bounds

| | |
|---|---|
| **Stops** | [`pid_wrap`](../../attack/pid_wrap/) |
| **Protects** | identity: one PID = one process |
| **Called from** | `process_create()` and `process_kill()` in [core/process](../../../core/process/process.c) |

## The rule

```
0 < pid < 0xFFFFFF00
```

## Why

`next_pid` only counts up: 1, 2, 3, ... After `0xFFFFFFFF` a 32-bit
counter wraps back to 0, then 1, and PID 1 is still alive. Two processes
would share a PID, and `process_get_by_pid()` would quietly return the
wrong one.

The guard gives up 256 PIDs at the top, so the counter stops before it
can wrap. "No more PIDs" is a clear error; a reused PID is a silent one.
PID 0 always means "no process", so it is refused too.

## Uses

- `sec_log()` from the security lab

## Try it

```
seclab verify pid_bounds
seclab attack pid_wrap live     # probes the edge values
```
