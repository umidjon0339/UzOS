# Defence: reaper

| | |
|---|---|
| **Stops** | [`zombie_leak`](../../attack/zombie_leak/) |
| **Protects** | resource: slots and 4 KiB stacks come back |
| **Called from** | `scheduler_tick()` in [core/scheduler](../../../core/scheduler/scheduler.c), every timer tick |

## The rule

1. Only **zombies** are freed.
2. Never the **running** process.

## Why

When a process ends, `process_exit()` cannot free its own stack: it is
still running on it, and freeing it would pull the floor away. So it
only marks itself `ZOMBIE` and switches away for good.

The reaper frees zombies later. In ring 0 the timer interrupt runs on
the stack of whichever process it interrupted, which is the running one.
Rule 2 skips exactly that process, so the reaper never frees the stack
it is on. The zombie is freed on a later tick, once another process runs.

The reaper turns interrupts off while it works, so a timer tick in the
middle cannot free the same slot twice.

## Uses

- `kfree()` from memory_management, to return the stack
- `scheduler_get_current_process()`, to know who is running
- `cpu_irq_save()` / `cpu_irq_restore()` from arch/x86/cpu

## Try it

```
seclab verify reaper
seclab attack zombie_leak live   # 4 workers exit; slots come back
ps
```
