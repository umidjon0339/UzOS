# Defence: path_length

| | |
|---|---|
| **Stops** | [`long_path`](../../) |
| **Protects** | resource: path buffers (`FS_PATH_MAX` = 256) |
| **Called from** | `ramdisk_create()`, `ramdisk_move()`, `ramdisk_exchange()` |

## The rule

The full path of every node, and of everything below a moved
directory, stays shorter than 256 bytes. Creating or moving something
that would break this is refused.

## Try it

```
seclab verify path_length
```
