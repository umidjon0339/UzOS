# Attack: huge_file

| | |
|---|---|
| **Stopped by** | [`size_cap`](defence/size_cap/) |
| **Breaks** | resource: the RAM disk lives in the kernel heap |
| **Severity** | high |

## What it does

`truncate(file, 2 GiB)` asks for the whole size in one call: on a RAM
disk, one kmalloc of the entire kernel heap. Everything else then fails
to allocate.

## In UzOS

`live` creates `/seclab_huge` and calls the real `truncate()` with 2 GiB
and with 64 KiB + 1: both refused; 100 bytes works.

## Try it

```
seclab attack huge_file live
truncate -s 1M x       # refused: over 64 KiB
```
