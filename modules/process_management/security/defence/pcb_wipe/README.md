# Defence: pcb_wipe

| | |
|---|---|
| **Stops** | [`uninit_pcb`](../../attack/uninit_pcb/) |
| **Protects** | disclosure: a new process never sees an old one's data |
| **Called from** | `process_init()` and `process_create()` in [core/process](../../../core/process/process.c) |

## The rule

Zero the whole `process_t` before writing any field of the new process.

## Why

A process slot is reused many times. `process_create()` only fills some
fields (pid, name, uid, stack). The rest (`cwd`, `fd_table`,
`fd_offset`, `sleep_ticks`, ...) would keep whatever the last owner
left. That leaks data, such as which files it had open, and a stale
`cwd` pointer is a crash waiting to happen.

Wiping first means every field is either set by the new owner or zero.

## Uses

- `memset()` from lib/string

## Try it

```
seclab verify pcb_wipe          # fills a fake slot with 0xAB, wipes, checks
seclab attack uninit_pcb live
seclab repair
```
