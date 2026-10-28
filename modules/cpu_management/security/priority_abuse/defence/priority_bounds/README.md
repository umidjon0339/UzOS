# Defence: priority_bounds

| | |
|---|---|
| **Stops** | [`priority_out_of_range`](../../attack/priority_out_of_range/) |
| **Protects** | trust and privilege: nice values and policies are real ones, set by who may set them |
| **Called from** | `priority_set_nice()` in [core/priority](../../../core/priority/priority.c) and `policy_set()` in [core/policy](../../../core/policy/policy.c), used by the syscalls `setpriority`, `sched_setscheduler`, `sched_setparam`, `sched_setattr` and the commands `nice`, `renice`, `chrt`, `htop` |

## The rules

A nice value:

```
1. -20 <= nice <= 19                          a real nice value
2. the target exists, not a system process    idle, kernel_main, statusd keep theirs
3. CAP_PRIORITY (root) may set any nice on any process
4. a user: only its own processes, and only UP (nicer),
   or down to 20 - RLIMIT_NICE (0 by default: not below its current nice)
```

A scheduling policy:

```
1. a policy UzOS knows (no SCHED_DEADLINE), priority in its range:
   1..99 for SCHED_FIFO / SCHED_RR, 0 for OTHER / BATCH / IDLE
2. the target exists, not a system process
3. CAP_PRIORITY (root) may set any policy on any process
4. a user: only its own processes; real-time only up to its
   RLIMIT_RTPRIO (0 by default: no real-time at all)
```

## Why

The numbers come from a user program, so they are checked like any
other outside input:

| Request | What it would do |
|---|---|
| nice 1000 / rt priority 1000 | ranks above everything: nothing could take the CPU back |
| nice -500 | not a nice value at all |
| a user sets nice -20 | jumps ahead of every other user |
| a user makes itself SCHED_FIFO 99 | runs before every normal process; only RT throttling stops it |
| a user renices the shell | slows the OS for everybody |

Rule 4 is Unix's `nice`: anyone may be **nicer**, only root (or a raised
RLIMIT_NICE / RLIMIT_RTPRIO, see `ulimit -e` / `-r`) may be greedier.
The checks live in `priority_set_nice()` and `policy_set()`, not in a
syscall or a command, so every caller passes through them.

## Uses

- `NICE_MIN` / `NICE_MAX` from [core/priority](../../../core/priority/)
- `policy_valid`, `policy_priority_min/max`, `policy_is_realtime` from [core/policy](../../../core/policy/)
- `RLIMIT_NICE`, `RLIMIT_RTPRIO` from [core/rlimit](../../../core/rlimit/)
- `process_t` (`uid`, `is_system`, `nice`, `rlim_cur`) from process_management
- `caps_has(uid, CAP_PRIORITY)` from security_management [core/caps](../../../../security_management/core/caps/)
- `audit_log()` from security_management (core/audit)

## Try it

```
seclab attack priority_out_of_range live
renice 19 -p 2                   # refused: kernel_main is a system process
su guest
chrt -f 10 ps                    # refused: real-time needs root
nice -n -5 ps                    # warned, runs with nice 0 (like Linux)
seclab verify priority_bounds
```
