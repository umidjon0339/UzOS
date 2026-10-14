# Attack: fork bomb

**Breaks:** `resource` · **Severity:** CRITICAL · **Paired defence:** [`slot_quota`](../../defence/slot_quota/README.md)

## The idea

Process slots are a finite resource. A process that calls `fork()` in a
loop, where every child also calls `fork()`, doubles the number of
processes on every round. The table fills in a handful of iterations.

The interesting part is not the crash, it is the *recovery*: once every
slot is taken, no new process can start - including the shell you would
use to clean up. A fork bomb turns a temporary load spike into a
permanent denial of service.

## How it works here

`process_create()` is the kernel's fork primitive. The attack calls it
in a loop with `uid = 1`, so it is treated as an unprivileged user.
A real bomb would recurse; we cap a live run at `ATTACK_BUDGET` (16)
processes and never fork from a child, so the demo cannot wedge the box.

`simulate()` only counts free slots in the process table - it is
strictly read-only. `launch()` really creates processes. `cleanup()`
kills anything named `bomb` so the next test starts clean.

## Try it

```
seclab attack fork_bomb          # safe: reports what would happen
seclab attack fork_bomb live     # real: creates processes
seclab log                       # see the denials, if any
seclab repair                    # clean up the leftovers
```

## What you should see

A live run is stopped after 8 processes by the per-user cap, and the
event log gains `slot_quota` entries. The system stays usable: the
shell keeps running and root can still create processes.

## Where the defence hooks in

`process_create()` calls `def_slot_quota_check(uid)` before taking a
slot. See [`def_slot_quota.c`](../../defence/slot_quota/def_slot_quota.c).
