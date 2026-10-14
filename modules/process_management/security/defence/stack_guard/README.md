# Defence: stack guard

**Counters:** [`stack_overflow`](../../attack/stack_overflow/README.md) · **Restores:** `isolation`

## The idea

A process owns exactly `PROCESS_STACK_SIZE` (4 KB) of stack, so its
stack pointer must stay in `[stack_base, stack_base + size)`. If `esp`
has left that range, the process already wrote outside its own stack,
over a heap object or another process's data. The guard cannot undo
that write, but it refuses to switch into the broken process, so the
damage stops spreading.

The whole policy, quoted from [`def_stack_guard.c`](../../defence/stack_guard/def_stack_guard.c):

```c
static int stack_pointer_ok(uint32_t esp, uint32_t base) {
    if (base == 0) return 1;                        /* unguarded    */
    if (esp < base) return 0;                       /* past the end */
    if (esp >= base + PROCESS_STACK_SIZE) return 0; /* outside all  */
    return 1;
}
```

| Check | Verdict | Why |
|---|---|---|
| `base == 0` | allow | no stack was allocated, so there is nothing to compare; the idle task is one of these |
| `esp < base` | refuse | below the bottom - recursion has escaped |
| `esp >= base + size` | refuse | at or above the top - a valid `esp` points at the last word pushed |
| anything else | allow | inside the 4 KB the process owns |

## Why the policy is a separate function

`stack_pointer_ok()` depends on two numbers and nothing else, so
`selftest()` can test the boundary cases (`base`, `base - 1`,
`base + PROCESS_STACK_SIZE`) directly instead of corrupting a real
stack. The wrapper only adds the `sec_log()` call seen in `seclab log`.

## Where the kernel calls it

```c
/* scheduler.c - scheduler_yield(), before switch_task() */
if (current_process && !def_stack_guard_check(current_process)) {
    sec_log("stack_guard", "stack pointer left its own stack");
    current_process->state = PROCESS_STATE_ZOMBIE;
}
```

It runs while we are still on a sane stack, and the ring 3 entry path
uses `def_user_stack_ok(esp)` for the same reason. Note the limitation:
this is a software check taken at switch time, so a write between two
switches is not stopped. A guard page below each stack would fault on
the first bad write, and needs per-process paging.

## Try it

```
seclab verify stack_guard             # BLOCKED = boundary cases hold
seclab attack stack_overflow live     # the attack asks the guard
seclab log                            # evidence the guard really ran
```
