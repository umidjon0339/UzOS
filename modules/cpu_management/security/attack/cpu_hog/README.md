# Attack: cpu_hog

| | |
|---|---|
| **Stopped by** | [`aging`](../../defence/aging/) |
| **Breaks** | liveness: no process may starve the others |
| **Severity** | high |

## What it does

With **strict** priorities the scheduler always picks the highest READY
process. A process at level 7 that never sleeps is always READY, so:

```
level 7   hog      ██████████████████████████  (spins: while (1);)
level 4   shell    ..........................  never runs again
level 1   victim   ..........................  never runs again
```

The OS looks frozen: the shell cannot even read the keyboard. Getting
level 7 needs root (see `priority_bounds`), but root programs have bugs
too, and a stuck driver thread at high priority does exactly this.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | the arithmetic: how long the victim and the shell wait with aging |
| `live` | starts `hog` at 7 and `victim` at 1 (counts its turns), sleeps the shell for 1 s, then reports who got the CPU; the hog is killed before printing |

A run in QEMU:

```
victim got the CPU 2 times in 1000 ms
the shell slept 1000 ms, got the CPU back after 1300 ms
```

The shell is 300 ms late: it waited at level 4 until aging lifted it to
7 (3 levels × 100 ms). Without aging it would never come back.

## Uses

- `process_create()` / `process_kill()` from process_management
- `priority_set()` from [core/priority](../../../core/priority/)
- `sys_sleep()`, `sys_yield()`, `timer_uptime_ms()`

## Try it

```
seclab attack cpu_hog
seclab attack cpu_hog live
```
