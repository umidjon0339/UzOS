# Defence: PCB wipe

**Counters:** [`uninit_pcb`](../../attack/uninit_pcb/README.md) · **Restores:** `disclosure`

## The idea

Zero a process slot at the moment it is handed to a new process, so no
field can survive from the previous owner.

```c
void def_pcb_wipe(process_t *p) {
    if (!p) return;
    memset(p, 0, sizeof(process_t));
    p->state = PROCESS_STATE_FREE;
}
```
Three details matter:

| Detail | Why |
|---|---|
| clear the whole struct | `memset` does not know the field list, so a field added later is wiped too and cannot quietly become a new leak |
| then set `state = FREE` | all-zero is not a valid state (`READY` is 0), and a slot that is not `FREE` would look occupied |
| clear *before* filling | every field is then either the new owner's or zero - one rule that is easy to reason about |

The NULL check keeps this safe to call before a slot has been found.

## Why there is no separate policy function

Other defences answer a question ("may this user create a process?")
with a pure function of a few numbers, which is easy to test alone.
This one always wipes, so there is nothing to decide - a policy
function would only add a layer. The lesson is different:
**clear before you fill, at the single place a slot changes hands**.

## Where the kernel calls it

```c
/* modules/process_management/core/process.c */
process_t *p = &process_table[slot];

/* DEFENCE pcb_wipe: no field of the previous owner may survive. */
def_pcb_wipe(p);

p->pid   = next_pid++;
```

Delete that call and the leak returns. `process_init()` and
`process_fork()` also wipe, so every handover uses the same rule.

## Try it

```
seclab verify pcb_wipe     # BLOCKED = the selftest holds
seclab attack uninit_pcb live
seclab info uninit_pcb
```

The live run should report `inherited stale fields: 0`. This defence
calls no `sec_log()`, so `seclab log` stays empty and the verdict is
"not refused" - the count of zero is the evidence.
