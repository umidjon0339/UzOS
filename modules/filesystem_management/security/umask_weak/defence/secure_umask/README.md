# Defence: secure_umask

| | |
|---|---|
| **Stops** | [`umask_weak`](../../) |
| **Protects** | integrity: nobody's new file is writable by everyone |
| **Called from** | `ramdisk_create()` in [core/ramdisk](../../../../core/ramdisk/ramdisk.c), for every new file and directory |

## The rule

New mode = `rw/rw & ~umask`, and the **others-write bit is always
cleared**. `umask 0` cannot undo it; an explicit `chmod 66` later can,
because then someone decided it on purpose.

## Try it

```
seclab verify secure_umask
```
