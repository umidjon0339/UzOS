# Defence: no_cycle

| | |
|---|---|
| **Stops** | [`dir_cycle`](../../attack/dir_cycle/) |
| **Protects** | consistency: the directory tree stays a tree |
| **Called from** | `ramdisk_move()` in [core/ramdisk](../../../core/ramdisk/ramdisk.c) |

## The rule

A directory may not be moved into **itself** or into any directory
**below** it. To check, walk up from the destination to the root; if
you pass the directory being moved, refuse.

## Why

```
before:  / -> a -> b          mv /a /a/b
after:   a's parent is b, b's parent is a      (a loop)
         nothing connects them to / any more
```

The whole subtree is cut off from the root, so its files are lost. Worse,
code that walks up to the root, like `pwd`, follows `parent` pointers
round and round forever.

## Uses

- `vfs_is_ancestor()` from core/vfs
- `sec_log()` from the security lab

## Try it

```
mkdir a
mkdir a/b
mv a a/b                         # refused
seclab attack dir_cycle live
```
