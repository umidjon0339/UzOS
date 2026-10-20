# Defence: transfer_bounds

| | |
|---|---|
| **Stops** | [`oversized_transfer`](../../attack/oversized_transfer/) |
| **Protects** | trust: a buffer from a program is checked before copying |
| **Called from** | `sys_read` and `sys_write` in [syscalls/](../../../syscalls/) |

## The rules

Nothing to copy (`count == 0`) is always fine. Otherwise:

1. `count <= 16 KiB`
2. `buf` is not in the NULL page
3. `buf + count` does not wrap past 4 GiB (checked as
   `count - 1 <= 0xFFFFFFFF - buf`, which cannot overflow)

## Why

`read(fd, buf, count)` copies `count` bytes **to** `buf`. A `NULL`
`buf`, a huge `count`, or a range like `0xFFFFFFF0 + 32` (which wraps
round to address `0x10`) makes the kernel write over memory the caller
never owned.

For calls from ring 3 there is one more layer: the `int 0x80` handler
first checks that the whole buffer is mapped **user** memory
(`def_user_range_ok`, memory_management). Kernel callers like `cat`
pass kernel buffers, so they only get these rules.

## Uses

- `NULL_PAGE_END` (memory_management layout), `IO_MAX_TRANSFER` from core/fd
- `sec_log()` from the security lab

## Try it

```
seclab attack oversized_transfer live
```
