# Defence: file_range

| | |
|---|---|
| **Stops** | [`file_offset_wrap`](../../attack/file_offset_wrap/) |
| **Protects** | consistency: a write stays inside its own file |
| **Called from** | `ramdisk_write()` in [core/ramdisk](../../../core/ramdisk/ramdisk.c) |

## The rule

```
offset <= limit   and   size <= limit - offset        (limit = 64 KiB)
```

## Why

The obvious check adds first:

```c
if (offset + size <= limit) ...      // 0xFFFFFFF0 + 0x20 = 0x10: passes!
```

In 32 bits the sum wraps around to a small number, the check passes,
and the copy writes at `data + 0xFFFFFFF0`: 4 GiB away from the file,
into some other part of memory. Subtracting instead of adding cannot
wrap.

## Uses

- `sec_log()` from the security lab

## Try it

```
seclab attack file_offset_wrap live
```
