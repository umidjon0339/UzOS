# Defence: depth_limit

| | |
|---|---|
| **Stops** | [`deep_nesting`](../../) |
| **Protects** | liveness: kernel stacks of every tree walker |
| **Called from** | `ramdisk_create()`, `ramdisk_move()`, `ramdisk_exchange()`; `tree`, `find`, `du` also stop at `FS_DEPTH_MAX` |

## The rule

No node sits deeper than **32** levels below `/`.

## Try it

```
seclab verify depth_limit
```
