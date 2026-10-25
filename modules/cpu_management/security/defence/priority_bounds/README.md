# Defence: priority_bounds

| | |
|---|---|
| **Stops** | [`priority_out_of_range`](../../attack/priority_out_of_range/) |
| **Protects** | trust and privilege: priorities are real levels, set by who may set them |
| **Called from** | `priority_set()` in [core/priority](../../../core/priority/priority.c), used by syscall `setpriority` and the `nice` command |

## The rule

```
1. 1 <= level <= 7                       a real level (0 is the idle task's)
2. the target exists, not a system process   idle, kernel_main, statusd keep theirs
3. CAP_PRIORITY (root) may set any level on any other process
4. a user: only its own processes, and only DOWN (level <= current)
```

## Why

The level comes from a user program, so it is checked like any other
outside input:

| Request | What it would do |
|---|---|
| level 1000 | ranks above the aging cap: nobody could ever take the CPU back |
| level -5 | below idle; 4 billion when read as unsigned |
| user raises itself to 7 | jumps ahead of every other user |
| user lowers the shell | slows the OS for everybody |

Rule 4 is Unix's `nice`: anyone may be **nicer** (lower), only root may
be greedier. The check lives in `priority_set()`, not in the syscall
or the command, so every caller passes through it.

## Uses

- `PRIORITY_MIN` / `PRIORITY_MAX` from [core/priority](../../../core/priority/)
- `process_t` (`uid`, `is_system`, `priority`) from process_management
- `caps_has(uid, CAP_PRIORITY)` from security_management [core/caps](../../../../security_management/core/caps/)
- `audit_log()` from security_management (core/audit)

## Try it

```
seclab attack priority_out_of_range live
nice 2 1                         # refused: kernel_main is a system process
seclab verify priority_bounds
```
