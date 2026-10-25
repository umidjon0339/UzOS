# Defence: kill_guard

| | |
|---|---|
| **Stops** | [`kill_system`](../../attack/kill_system/) |
| **Protects** | privilege: a caller cannot destroy what it does not own |
| **Called from** | `process_kill()` in [core/process](../../../core/process/process.c) |

## The rule

1. The target must exist.
2. **System processes** (`idle`, `kernel_main`) can never be killed.
   `scheduler_init()` marks them with `is_system = 1`.
3. Whoever holds **CAP_KILL** (root) may kill any other process; a
   user only its own.

## Why

A check in the `kill` shell command would only guard one door. A
syscall or another module calling `process_kill()` directly would walk
past it. So the guard lives **inside** `process_kill()`, the one place
every kill must go through.

Killing `kernel_main` would kill the shell. Killing `idle` would leave
the scheduler with nothing to run when everyone sleeps.

## Uses

- `process_t.is_system` and `process_t.uid` from the process table
- `caps_has(uid, CAP_KILL)` from security_management [core/caps](../../../../security_management/core/caps/)
- `audit_log()` from security_management (core/audit), to record each refusal

## Try it

```
ps                     # idle and kernel_main show "(system)"
kill 1                 # refused
seclab attack kill_system live
audit
```
