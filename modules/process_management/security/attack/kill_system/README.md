# Attack: kill_system

| | |
|---|---|
| **Stopped by** | [`kill_guard`](../../defence/kill_guard/) |
| **Breaks** | privilege: nobody may destroy the kernel's own tasks |
| **Severity** | critical |

## What it does

UzOS has two system processes: `idle` (PID 1) and `kernel_main` (PID 2,
the shell). An early version protected PID 1 only inside the `kill`
shell command. But `process_kill()` is a normal kernel function: a
syscall or any module can call it directly, and the shell check never
runs.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the system processes that would be targets |
| `live` | calls `process_kill()` on each one (never on itself) |

With `kill_guard` on, every call is refused inside `process_kill()`.

## Uses

`process_kill()` and the process table from core/process,
`scheduler_get_current_process()` from core/scheduler.

## Try it

```
seclab attack kill_system live
seclab log
```
