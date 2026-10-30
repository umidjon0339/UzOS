# Defence: pcb_generation

| | |
|---|---|
| **Stops** | [`use_after_exit`](../../) |
| **Protects** | consistency: a saved process reference |
| **Called from** | core/pidfd: every use of a pidfd (`pidfd_send_signal`, `pidfd_getfd`, `waitid(P_PIDFD)`) |

## The rule

A reference is `{slot, pid, generation}` (`pcb_ref_t`). It is valid
only while the slot is used **and** still holds that pid and that
generation. `process_create()` counts a slot's generation up on every
new life; the counter lives outside the PCB, so `pcb_wipe` cannot
reset it.

## Why

Slots are reused; pointers are not updated. Linux keeps a reference
count on `task_struct` / `struct pid` for the same reason.

## Try it

```
seclab verify pcb_generation
seclab attack use_after_exit live
```
