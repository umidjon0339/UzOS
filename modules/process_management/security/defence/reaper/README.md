# Defence: reaper

**Counters:** [`zombie_leak`](../../attack/zombie_leak/README.md) · **Restores:** `resource`

## The idea

A dead process is a zombie: it never runs again, but its slot and its
4 KB stack are still held. `INV_RESOURCE` means giving both back.

Two rules, checked for every slot on every timer tick:

| Rule | Meaning | Why |
|---|---|---|
| only zombies | `state == PROCESS_STATE_ZOMBIE` | a live process must keep its stack |
| never ourselves | `pid != current_pid` | freeing the stack we stand on pulls the floor out from under us |

The second rule is the subtle one. Freeing the stack the CPU is
standing on corrupts the freeing code itself. The reaper runs from the
timer interrupt, so the CPU is on an interrupt stack - that is why the
tick is the right place for this work.

The whole policy, quoted from `def_reaper.c`:

```c
static int reapable(process_t *p, uint32_t current_pid) {
    if (!p) return 0;
    if (p->state != PROCESS_STATE_ZOMBIE) return 0;
    if (p->pid == current_pid) return 0;   /* that is our own floor */
    return 1;
}
```

`def_reap_zombies()` then frees the stack, tells the parent through
`def_child_release()`, and marks the slot `PROCESS_STATE_FREE`.

## Why the policy is a separate function

`reapable(p, current_pid)` is pure: one slot and one pid in, a verdict
out, no side effects. So `def_reaper_selftest()` can build fake
`process_t` values on the stack and test the boundary cases - running,
sleeping, someone else's zombie, our own zombie, a null slot - without
touching the real table. Keep the decision pure, the free elsewhere.

## Where the kernel calls it

```c
/* modules/process_management/core/scheduler.c */
def_reap_zombies();                             /* in scheduler_tick() */

/* modules/process_management/core/process.c */
if (self) self->state = PROCESS_STATE_ZOMBIE;   /* in process_exit()  */
```

Delete the tick call and zombies pile up again: the leak is back.

## Try it

```
seclab verify reaper          # BLOCKED = the boundary cases hold
seclab attack zombie_leak live
seclab log
```
