# Attack: use_after_unlink

| | |
|---|---|
| **Stopped by** | [`inode_refcount`](defence/inode_refcount/) |
| **Breaks** | consistency: a name or a descriptor always leads to live memory |
| **Severity** | high |

## What it does

With hard links, a file is the inode its names point at. If removing ONE name freed it, the other names and every open descriptor would point at freed kernel memory.

## In UzOS

`live`: `/seclab_keep` ("STILL HERE") gets a second name `/seclab_keep2`; the first name is removed, the file is read through the second; then the second is opened, removed, and the descriptor is read; then it is closed (and only now freed).

## Try it

```
seclab attack use_after_unlink live
seclab repair
```
