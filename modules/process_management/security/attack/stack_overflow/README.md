# Attack: stack overflow

**Breaks:** `isolation` · **Severity:** CRITICAL · **Paired defence:** [`stack_guard`](../../defence/stack_guard/README.md)

## The idea

Every process gets a fixed 4 KB stack, and `push` moves downwards. A
recursive function with no base case keeps pushing past the bottom of
that stack into whatever the heap allocated next.

The process does not crash. Some other object's fields quietly change,
and the damage only shows up later somewhere unrelated. Silent
corruption is worse than a fault, because nothing reports it.

## How it works here

- `simulate()` reads the running process's `stack_base` and `esp` and
  prints how much room is left below `esp`. It is read-only.
- `launch()` never overflows a real stack. It recurses `SAFE_DEPTH` (8)
  frames of `volatile` locals to measure a real frame, then builds a
  local `process_t` called `ghost` whose `esp` is one push below its
  own base, and asks `def_stack_guard_check(&ghost)` for the verdict.
  That is the exact function the scheduler calls.
- `cleanup()` is `0`: nothing was created or modified, so there is
  nothing to undo.

## Where the defence hooks in

```c
/* modules/process_management/core/scheduler.c */
if (current_process && !def_stack_guard_check(current_process)) {
    current_process->state = PROCESS_STATE_ZOMBIE;
}
```

The check runs on every context switch, so a stack pointer that has
already escaped its own stack is retired instead of being switched
into. See [`def_stack_guard.c`](../../defence/stack_guard/def_stack_guard.c).

## Try it

```
seclab attack stack_overflow          # safe: shows the margin left
seclab attack stack_overflow live     # measures a frame, asks the guard
seclab verify stack_guard             # the boundary cases
seclab log                            # the refusal, as evidence
```

## What you should see

`launch()` reports a frame size and how many such frames fit in 4 KB -
typically a few hundred, which is exactly why deep recursion is
dangerous. The guard then answers `REFUSED`, and `seclab log` gains a
`stack_guard` entry: "stack pointer left its own stack".

Because the attack only measures and never corrupts, `seclab repair`
has nothing to clean up. The stronger fix is a guard page - unmap the
page below each stack so the CPU faults on the first bad write. That
needs per-process paging; this is the cheap half that works today.
