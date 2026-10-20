# Attack: uninit_pcb

| | |
|---|---|
| **Stopped by** | [`pcb_wipe`](../../defence/pcb_wipe/) |
| **Breaks** | disclosure: no stale or foreign data |
| **Severity** | high |

## What it does

The process table has 64 slots that are reused again and again.
`process_create()` writes the fields it needs (pid, name, uid, stack).
Any field it does **not** write (`cwd`, `fd_table`, `fd_offset`, ...)
keeps the previous owner's value. The new process then "has" files it
never opened, and a stale `cwd` pointer can crash the filesystem code.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | counts free slots that still hold old data |
| `live` | writes fake old data (`cwd`, fd 3) into the next free slot, creates a process there, counts what survived |
| `repair` | kills the probe process |

With `pcb_wipe` on, the count is 0.

## Uses

`process_create()`, `process_kill()` and the process table from core/process.

## Try it

```
seclab attack uninit_pcb live
seclab repair
```
