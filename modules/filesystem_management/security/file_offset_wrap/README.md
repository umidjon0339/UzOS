# Attack: file_offset_wrap

| | |
|---|---|
| **Stopped by** | [`file_range`](defence/file_range/) |
| **Breaks** | consistency: a write stays inside its own file |
| **Severity** | high |

## What it does

A write gives an `offset` and a `size`. The obvious check adds them:

```c
if (offset + size <= limit)       // 0xFFFFFFF0 + 0x20 = 0x10  -> passes
    memcpy(data + offset, ...);   // data + 0xFFFFFFF0: 4 GiB away
```

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows the sum the naive check computes |
| `live` | creates a temporary file, tries the wrapping write and a write one byte past 64 KiB (both refused), a normal write (allowed), then deletes the file |

## Uses

`ramdisk_create()`, `ramdisk_delete()` from core/ramdisk, `vfs_write()` from core/vfs.

## Try it

```
seclab attack file_offset_wrap live
```
