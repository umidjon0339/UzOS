# Defence: busy_check

| | |
|---|---|
| **Stops** | [`delete_open_file`](../../) |
| **Protects** | consistency: no pointer to freed memory |
| **Called from** | `ramdisk_delete()` in [core/ramdisk](../../../../core/ramdisk/ramdisk.c) |

## The rules

A file or directory may be deleted only if:

1. no descriptor has it open (`open_count == 0`)
2. it is not a directory that still has entries
3. it is not the root
4. no process has it as its current directory

## Why

Deleting calls `kfree()` on the node. Anything still pointing at it then
points at **freed memory**. This is called *use after free*:

```
fd 3 -> node "notes"     rm notes  ->  kfree(node)
                         kmalloc(...) reuses that memory for, say, a stack
read(3, ...)             -> reads the stack as if it were the file
```

The old `rm` did exactly this. Directories with entries were freed too,
leaking every file inside, and `rm` of the current directory left the
shell standing in freed memory.

`open_count` is raised by `vfs_open()` and lowered by `vfs_close()`. io
calls them for every `open` and `close`, and when a process ends.

## Uses

- `vfs_node_t.open_count`, `fs_root` from core/vfs
- the process table (process_management), to find current directories
- `audit_log()` from security_management (core/audit)

## Try it

```
seclab attack delete_open_file live
```
