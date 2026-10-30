# Defence: size_cap

| | |
|---|---|
| **Stops** | [`huge_file`](../../) |
| **Protects** | resource: kernel memory |
| **Called from** | `sys_truncate`, `sys_ftruncate`, `sys_fallocate`, before `ramdisk_resize()` allocates |

## The rule

1. No size above `RAMDISK_MAX_FILE_SIZE` (64 KiB).
2. No size above the caller's `RLIMIT_FSIZE` (`ulimit -f`).

## Try it

```
seclab verify size_cap
```
