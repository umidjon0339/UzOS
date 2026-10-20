# Defence: mmap_page_count

| | |
|---|---|
| **Stops** | [`mmap_length_wrap`](../../attack/mmap_length_wrap/) |
| **Protects** | resource: every mapped byte has a real page behind it |
| **Called from** | `sys_mmap()` and `sys_munmap()` in [syscalls/](../../../syscalls/) |

## The rules

1. `0 < length <= 16 MiB`
2. `pages = length / 4096`, plus 1 if there is a remainder

## Why

The usual rounding formula is

```c
pages = (length + 4095) / 4096;
```

For `length = 0xFFFFFFFF` the addition wraps to `0xFFF`, and `pages`
becomes **0**. mmap would then report success with no pages behind the
address, and the first write would fault, or hit someone else's memory.

Rule 2 divides first, so nothing can overflow. Rule 1 stops a single
call from asking for all of RAM.

## Uses

- `PAGE_SIZE` from core/layout
- `sec_log()` from the security lab

## Try it

```
seclab verify mmap_page_count
seclab attack mmap_length_wrap live
memtest                          # a normal mmap still works
```
