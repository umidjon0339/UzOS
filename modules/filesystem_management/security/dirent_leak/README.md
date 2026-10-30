# Attack: dirent_leak

| | |
|---|---|
| **Stopped by** | [`dir_read_check`](defence/dir_read_check/) |
| **Breaks** | disclosure: the names in a private directory are private |
| **Severity** | medium |

## What it does

A descriptor lives on after the rights it was opened with. User 1 opens
root's directory while it is readable; root then runs `chmod 60`. If
`getdents` trusted the old open, user 1 would keep seeing every name.

## In UzOS

`live`: a user 1 process opens `/seclab_private` (mode 64), root sets
60, the process calls the real `getdents64` again: refused.

## Try it

```
seclab attack dirent_leak live
seclab repair
```
