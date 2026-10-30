# Attack: bad_filename

| | |
|---|---|
| **Stopped by** | [`filename_check`](../../defence/filename_check/) |
| **Breaks** | trust: names are checked before use |
| **Severity** | high |

## What it does

Some names break the filesystem as soon as they exist:

| Name | Problem |
|---|---|
| `..` / `.` | can never be opened: they always mean parent / here |
| `a/b` | `/` splits a path, so the file is unreachable |
| `my file` | the shell splits arguments at spaces |
| (empty) | nothing to type at all |
| 300 characters | overflows the 128-byte name buffer while it is copied |

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the names and their problems |
| `live` | really calls `ramdisk_create()` with each name in `/`, and `vfs_lookup()` with a 300-character component. Anything created by mistake is deleted again. |

## Uses

`ramdisk_create()`, `ramdisk_delete()` from core/ramdisk, `vfs_lookup()` from core/vfs.

## Try it

```
seclab attack bad_filename live
```
