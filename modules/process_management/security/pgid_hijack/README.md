# Attack: pgid_hijack

| | |
|---|---|
| **Stopped by** | [`session_check`](defence/session_check/) |
| **Breaks** | isolation: a login's signals reach only its own processes |
| **Severity** | medium |

## What it does

A process group gets its terminal's signals (Ctrl+C, Ctrl+Z, SIGHUP at
logout), and `kill(-pgid)` signals the whole group. If `setpgid()` let
anyone into any group, a process could join another login's group, or
pull a stranger's process into its own and then `kill(-group)` it.

## In UzOS

`live`: user 1's `spy` (in a session of its own) asks to join the
shell's group, and to pull root's `root_job` into its group. Both are
refused. `proccalls session` tries `setpgid(0, 1)` from ring 3: refused.

## Try it

```
seclab attack pgid_hijack live
seclab repair
```
