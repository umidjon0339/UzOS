# Defence: inode_refcount

| | |
|---|---|
| **Stops** | [`use_after_unlink`](../../) |
| **Protects** | consistency: a name or a descriptor always leads to live memory |
| **Called from** | `ramdisk_delete()` (a name goes), `vfs_close()` (a descriptor goes) |

## The rule

A file's memory is freed only when **both** counts are 0: names (`nlink`) and open descriptors (`open_count`). Removing one name of a file that has others removes only that name; a file with no name left lives until its last descriptor closes.

## Try it

```
seclab verify inode_refcount
```
