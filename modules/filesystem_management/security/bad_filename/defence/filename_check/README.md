# Defence: filename_check

| | |
|---|---|
| **Stops** | [`bad_filename`](../../) |
| **Protects** | trust: names from outside are checked before use |
| **Called from** | `vfs_lookup()` / `vfs_lookup_parent()` in [core/vfs](../../../../core/vfs/vfs.c), `ramdisk_create()` / `ramdisk_move()` in [core/ramdisk](../../../../core/ramdisk/ramdisk.c) |

## The rules

| Check | Rule |
|---|---|
| every path component | 1 to 127 characters, so it fits the name buffer |
| a new or renamed name | also: not `.` or `..`, no `/`, only visible characters |

## Why

| Bad name | Problem |
|---|---|
| 200 characters | copying it into the 128-byte `name` buffer overflows it |
| `..` | a file with that name can never be opened: `..` always means "parent" |
| `a/b` | `/` always splits a path, so this file is unreachable |
| `my file` | the shell splits arguments at spaces: you could never type it |
| control codes | printing the name would send terminal commands |

## Uses

- `FS_NAME_MAX` from core/vfs
- `audit_log()` from security_management (core/audit)

## Try it

```
seclab attack bad_filename live
touch ..                         # refused
```
