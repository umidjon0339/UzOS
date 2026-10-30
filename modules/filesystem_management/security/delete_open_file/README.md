# Attack: delete_open_file

| | |
|---|---|
| **Stopped by** | [`busy_check`](defence/busy_check/) |
| **Breaks** | consistency: nothing points at freed memory |
| **Severity** | critical |

## What it does

`rm` frees the node with `kfree()`. Three things can still point at it:

| Still pointing | What happens after the free |
|---|---|
| an open descriptor | `read(fd)` reads whatever `kmalloc()` put there next |
| the files inside a directory | lost: nothing points at them any more (leak) |
| a process's current directory | the shell "stands" in freed memory |

This class of bug is called **use after free**, one of the most common
security holes in real kernels.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the three cases |
| `live` | builds each case for real in `/` and calls `sys_unlink()` / `sys_rmdir()`: an open file, a directory with a file in it, and the current directory. Then cleans up properly. |

## Uses

`ramdisk_create()` from core/ramdisk, `vfs_open()` / `vfs_close()` from
core/vfs, `sys_unlink()` from syscalls/unlink.

## Try it

```
seclab attack delete_open_file live
ls                               # no seclab_* entries left
```
