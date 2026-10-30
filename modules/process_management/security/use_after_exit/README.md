# Attack: use_after_exit

| | |
|---|---|
| **Stopped by** | [`pcb_generation`](defence/pcb_generation/) |
| **Breaks** | consistency: a reference names one process, for ever |
| **Severity** | high |

## What it does

The process table has 64 slots, and they are reused. Kernel code that
keeps a `process_t *` while it sleeps (a wait, a pidfd, a debugger) can
wake up after that process ended and its slot went to a new one. The
old pointer then signals, inspects or reaps the **newcomer**.

## In UzOS

`live`: start `victim` (uid 1) and keep a reference, kill it, reap it,
start `newcomer` (root) until it lands in the same slot, then use the
old reference. The raw pointer names the newcomer; the checked one
(`def_pcb_ref_get`) is refused.

## Try it

```
seclab attack use_after_exit live
audit
seclab repair
```
