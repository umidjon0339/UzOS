# Attack: memory_peek

| | |
|---|---|
| **Stopped by** | [`vm_access_check`](defence/vm_access_check/) |
| **Breaks** | disclosure: a process's memory is its own |
| **Severity** | high |

## What it does

`process_vm_readv()` copies memory out of another process in one call.
Without a check, any user reads root's processes, or names a "remote"
address that is really the kernel and reads the kernel itself.

## In UzOS

[memory_peek.s](memory_peek.s) (ring 3, uid 1) reads 16 bytes from
`idle` (refused), from 0x00100000, the kernel's code, as if it were its
own (refused), and from its own memory (works).

## Try it

```
seclab attack memory_peek live
audit
```
