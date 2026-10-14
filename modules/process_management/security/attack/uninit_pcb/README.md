# Attack: uninitialised PCB

**Breaks:** `disclosure` · **Severity:** HIGH · **Paired defence:** [`pcb_wipe`](../../defence/pcb_wipe/README.md)

## The idea

A process slot is recycled many times during one boot. The kernel hands
a free slot to a new process and writes the fields it needs: pid, uid,
state, name and stack.

It does *not* write `cwd`, `fd_table` or `fd_offset`. If the slot is not
cleared first, those fields still hold the previous owner's values. The
new process - possibly a different user - is now holding a stale pointer
to someone else's working directory and open files. That is an
information leak, and dereferencing the old `cwd` is a crash waiting to
happen.

This is the classic "uninitialised structure" bug: forgetting that
*free* does not mean *empty*.

## How it works here

`launch()` finds the first free slot - the same one `process_create()`
will pick - creates exactly one process in it, and counts the fields
that `process_create()` never writes. A non-zero value there was
inherited from the old owner.

`simulate()` only counts free slots that still hold such stale fields,
so it is strictly read-only. The probe process just halts, and
`cleanup()` stops it, so a live run stays bounded to one process.

## Try it

```
seclab attack uninit_pcb          # safe: how many dirty slots exist
seclab attack uninit_pcb live     # real: reuse a slot, count the leak
seclab verify pcb_wipe            # the defence's own selftest
seclab repair                     # stop the leftover probe
```

## What you should see

The live run reports `inherited stale fields: 0` and `clean - the slot
was wiped first`, because `def_pcb_wipe()` zeroed the slot inside
`process_create()` before any field was written. Remove the wipe and
the same run reports the old owner's data instead.

`seclab log` stays empty for this attack, and `seclab` will say the
attack "was not refused". That is expected: the wipe is a repair, not a
refusal, so it records no event. The proof is the field count of zero,
plus `seclab verify pcb_wipe`.

## Where the defence hooks in

`process_create()` calls `def_pcb_wipe(p)` after it finds a free slot
and before it writes anything. `process_init()` wipes all 64 slots at
boot, and `process_fork()` wipes the child slot. See
[`def_pcb_wipe.c`](../../defence/pcb_wipe/def_pcb_wipe.c).
