# Defence: aging

| | |
|---|---|
| **Stops** | [`cpu_hog`](../../attack/cpu_hog/) |
| **Protects** | liveness: everyone who waits long enough gets a turn |
| **Called from** | `priority_effective()` / `priority_pick_next()` in [core/priority](../../../core/priority/priority.c), on every scheduling decision |

## The rule

```
effective = base + waited_ticks / AGING_TICKS        never above 7
            AGING_TICKS = 10  (100 ms at 100 Hz)

waited_ticks  +1 every tick the process is READY but not running
              = 0 when it gets the CPU
idle (0)      never aged: it must stay last
```

## Why

Strict priority scheduling is simple and fast, but it can **starve**
low processes forever. Aging is the classic fix: the longer a process
waits, the higher it ranks, until it ties with the top and the round
robin gives it a turn.

| Waiting process | Waits before it ties the hog at 7 |
|---|---|
| level 4 (the shell) | 3 levels × 100 ms = 300 ms |
| level 1 | 6 levels × 100 ms = 600 ms |

The priority still matters: high processes run far more often. Aging
only puts a **limit** on how long anyone waits.

Each time aging decides the winner (a process wins although a higher
base level was ready), it logs `aging: a starving process got the CPU`,
at most once a second.

## Uses

- `PRIORITY_IDLE` / `PRIORITY_MAX` from [core/priority](../../../core/priority/)
- `timer_ticks` (rate limit of the log), `audit_log()` from security_management (core/audit)

## Try it

```
seclab attack cpu_hog live
nice                             # EFF > PRI while a process waits
seclab verify aging
```
