# Attack: zombie_leak

| | |
|---|---|
| **Stopped by** | [`reaper`](../../defence/reaper/) |
| **Breaks** | resource: slots and stacks must come back |
| **Severity** | high |

## What it does

When a process function returns, the process is finished, but it cannot
free its own stack while still standing on it. It becomes a **zombie**:
it never runs again, yet still holds its slot and 4 KiB stack. An early
version of UzOS never freed zombies, so every short-lived process leaked
one slot for good.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | counts free and zombie slots |
| `live` | starts 4 processes that end at once, waits 0.2 s, compares free slots before and after |
| `repair` | waits for the reaper and counts what it freed |

With the reaper on, "before" and "after" are equal.

## Uses

`process_create()` from core/process, `timer_wait()` from device_management/timer.

## Try it

```
seclab attack zombie_leak live
ps                               # no "leak" zombies left
```
