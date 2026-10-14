# Attack: zombie leak

**Breaks:** `resource` · **Severity:** HIGH · **Paired defence:** [`reaper`](../../defence/reaper/README.md)

## The idea

Every process owns two finite things: a slot in the process table and
a 4 KB stack. When a process exits both must come back, otherwise the
machine runs out of processes even though nothing is running.

In the original kernel, `process_exit()` ended in an endless loop. A
process that returned from its entry point stayed in the table as a
dead slot, still holding its stack. Nothing printed an error. The
system just quietly refused to start anything new.

That is the lesson: a leak does not look like a crash. It looks like a
healthy system that has stopped accepting work.

## How it works here

`process_create()` builds a stack whose return address is
`process_exit`, so a worker that simply returns takes the exit path.
`leak_worker()` is empty on purpose: returning *is* the attack.

A live run creates `LEAK_WORKERS` (4) unprivileged processes, then
calls `timer_wait(20)`. Waiting matters: the reaper runs from the
timer tick, so ticks must happen before the slots can come back. The
run prints the free-slot count before and after, so you can see the
slots returned. `simulate()` only reads the table.

## Try it

```
seclab attack zombie_leak        # safe: shows the resource at risk
seclab attack zombie_leak live   # real: workers exit, reaper reclaims
seclab log                       # reaper / watchdog events
seclab verify reaper             # the policy boundary cases
seclab repair                    # wait out any leftover zombie
```

## What you should see

A live run leaks four slots by exiting, then shows the same free-slot
count as before: the reaper reclaimed every one.

One honest caveat: `seclab` counts *refusals*, and the reaper refuses
nothing - it reclaims afterwards. So the live verdict can read
"not refused" even though the slots came back. The before/after
numbers and `seclab verify reaper` are the real proof.

## Where the defence hooks in

`process_exit()` marks the slot a zombie instead of spinning, and
`scheduler_tick()` calls `def_reap_zombies()`. See
[`def_reaper.c`](../../defence/reaper/def_reaper.c).
