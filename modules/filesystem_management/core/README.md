# filesystem_management/core — only what the kernel itself runs, or what many folders share as state

filesystem_management is built from four kinds of folders, the same as
[cpu_management](../../cpu_management/core/README.md) and
[process_management](../../process_management/core/README.md):

| Folder | Runs when | Holds |
|---|---|---|
| `commands/<name>/` | you type the command | everything that command needs; helpers copied in, several files if needed |
| `syscalls/<name>/` | a program does `int 0x80` | everything that syscall needs; helpers copied in, several files if needed |
| `security/<attack>/` (defence inside) | `seclab`, or the kernel asks a defence "is this allowed?" | the attack, its defence/`<name>`/, a README each |
| `core/<part>/` | **all the time**, without anyone asking | the filesystem machinery and its state |

## The rule: what may live in core

A part lives in `core/` only if at least one of these is true:

1. **The kernel calls it by itself**: at boot, from another module (the
   ELF loader, `open()`, the shell's prompt), from an interrupt.
2. **It holds state that several folders must share.** There is ONE
   tree of nodes. If `mkdir` kept its own copy, `ls` would never see the
   new directory. Copying code is fine; copying state is a bug.

Everything else lives in the folder that uses it, copied into each
folder if several use it.

## The parts, and who calls them

| Part | What it is | Called by the kernel (rule 1) | Shared state (rule 2) |
|---|---|---|---|
| `vfs/` | nodes and inodes, path walking (`vfs_lookup`, following links), `vfs_read` / `vfs_write`, `vfs_stat` | every module that names a file: `open`, `exec`, the prompt | the root `fs_root` |
| `ramdisk/` | the driver behind `/`: files are kmalloc buffers | boot (`ramdisk_init`), every create / delete / move | the files and directories themselves |
| `procfs/` | `/proc`: files written when you read them | boot (`procfs_init`), every read of `/proc` | the `/proc` nodes |

Every file names what it uses at its top ("Uses: ...") and includes
it by its full path, with what it takes from it:

```c
#include "modules/filesystem_management/core/vfs/vfs.h"   // vfs_lookup
```
