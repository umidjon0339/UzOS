# Attack: user_ptr_wrap

| | |
|---|---|
| **Stopped by** | [`user_ptr_bounds`](../../defence/user_ptr_bounds/) |
| **Breaks** | trust: addresses from a program are checked first |
| **Severity** | high |

## What it does

A system call gets an address range `(start, size)` from a program. The
obvious check is `start + size <= END`. But in 32 bits:

```
0xFFFFF000 + 0x2000 = 0x00001000
```

The sum wraps around, the check passes, and the range really covers
the top of memory and the kernel at the bottom. The old check also
treated the kernel at 1 MiB as "user" memory.

A range can also be inside user space but **not mapped**. When the
kernel copies to it, the page fault happens in ring 0, which halts the
whole OS.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | runs the naive check on five crafted ranges (one is an unmapped user page) and shows which it lets through |
| `live` | offers the same ranges to `def_user_range_ok()`; all must be refused |

## Uses

`def_user_range_ok()` from security/defence/user_ptr_bounds.

## Try it

```
seclab attack user_ptr_wrap          # see the naive check fail
seclab attack user_ptr_wrap live
```
