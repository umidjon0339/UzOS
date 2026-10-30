# Attack: path_walk_bypass

| | |
|---|---|
| **Stopped by** | [`search_check`](defence/search_check/) |
| **Breaks** | privilege: a private directory hides everything below it |
| **Severity** | high |

## What it does

The plain path into a private directory is refused, so an attacker
tries side roads: in and out with `..`, or a directory descriptor
opened while the directory was still open (`fstatat`, `fchdir`).

## In UzOS

`live`: a user 1 process opens root's `/seclab_vault` while it is mode
64, root sets 60, the process tries all four roads with the real
syscalls: all refused.

## Try it

```
seclab attack path_walk_bypass live
seclab repair
```
