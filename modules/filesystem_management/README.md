# filesystem_management — files, directories and paths

A **filesystem** turns "a file called `/docs/notes`" into actual bytes
somewhere. UzOS splits that into two layers:

```
VFS (core/vfs)        the tree of nodes, paths, open counts
  driver (core/ramdisk)   where the bytes are kept: here, the kernel heap
  driver (core/procfs)    /proc: no bytes kept, the text is made on every read
```

```
filesystem_management/
├── core/
│   ├── vfs/      vfs.h/.c          nodes, open counts, path lookup, path of a node
│   ├── ramdisk/  ramdisk.h/.c      the RAM disk driver: create, delete, move, read, write
│   │             hello_elf.h       the bytes of /hello (userland/hello.elf)
│   └── procfs/   procfs.h/.c       the /proc driver: /proc, /proc/self, /proc/<pid>/
│                 proc_cpuinfo.c    /proc/cpuinfo   (numbers from cpu_management/core)
│                 proc_stat.c       /proc/stat
│                 proc_pid_status.c /proc/<pid>/status
├── commands/     fs_commands.h/.c + one folder per command:
│                 ls cd pwd mkdir touch rm cat write cp mv wc grep hexdump
├── syscalls/     mkdir/ unlink/ chdir/ getcwd/ rename/   (number + .h + .c)
└── security/     attack/<id>/  defence/<id>/   (4 pairs, each with a README)
```

## /proc: a second driver

`cat /proc/stat` goes through the same VFS as `cat /hello`. The
difference is the driver: the RAM disk's `read()` copies stored bytes,
procfs's `read()` WRITES the text right then from the kernel's live
counters (cpu_management/core: accounting, affinity, numa, ...).
`ls /proc` lists one directory per process that exists now; they are
made on lookup, never stored. The RAM disk never creates, deletes or
moves nodes it did not make (`ours()` in ramdisk.c), so `/proc` cannot
be broken with `rm` or `mv`.

## The tree and paths

```
/                      every node: name, flags (file/dir), size,
├── hello              parent, children, open_count, owner, mode
├── home
│   ├── root           private to root (mode 60)
│   └── guest          guest's home (mode 64)
└── docs
    ├── a.txt          "/docs/a.txt"   absolute: starts at /
    └── old            "old/../a.txt"  relative: starts at the current directory
```

`vfs_lookup()` walks a path one name at a time: `.` stays, `..` goes to
the parent (the root is its own parent), anything else is looked up in
the directory. Each process has its own current directory
(`process_t.cwd`), changed with `cd`.

`vfs_lookup_parent()` is for create/delete/rename: it finds the directory
that should hold the **last** name and hands back that name.

## Owners and permissions

Every node has an **owner** (a uid) and a **mode** (`64` = owner may
read and write, others may read). The rules live in
[user_management/core/perm](../user_management/core/perm/); this module
asks them before it touches anything:

| Where | Needs |
|---|---|
| `vfs_lookup`, every directory passed | read |
| `sys_chdir`, `ls` | read on the directory |
| `sys_mkdir`, creating with `open` | write on the parent directory |
| `sys_unlink`, `sys_rename` | write on the directory and owning the entry |

A new entry belongs to whoever created it. `ls` shows mode and owner;
`chmod` and `chown` change them. Root passes every check.

## The RAM disk

Each file's bytes are one `kmalloc()` buffer. It starts at 256 bytes and
doubles as the file grows, up to 64 KiB. Deleting frees it. Nothing
survives a reboot.

## Commands

| Command | What it does | Uses |
|---|---|---|
| `ls [path]` | list a directory (or show one file) | `vfs_lookup`, `vfs_readdir` |
| `cd [path]` / `pwd` | change / show the current directory | `sys_chdir`, `sys_getcwd` |
| `mkdir <path>` | new directory | `sys_mkdir` |
| `touch <path>` | new empty file | `sys_open(O_CREATE)` |
| `rm <path>` | delete a file or empty directory | `sys_unlink` |
| `cat <path>` | print a file | open / read / write(stdout) / close |
| `write <path> <text>` | replace a file's content | `sys_open(O_TRUNC)` |
| `cp <from> <to>` | copy (into a directory keeps the name) | read one fd, write another |
| `mv <from> <to>` | move or rename | `sys_rename` |
| `wc`, `grep <word>`, `hexdump [bytes]` | count / search / show bytes | read loops |

File contents go through io_management's `open`/`read`/`write`/`close`,
exactly like Unix programs.

## System calls

| # | Syscall | Arguments | Result |
|---|---|---|---|
| 9 | `mkdir` | EBX = path | 0 / -1 |
| 10 | `unlink` | EBX = path | 0 / -1 |
| 11 | `chdir` | EBX = path | 0 / -1 |
| 12 | `getcwd` | EBX = buffer, ECX = size | 0 / -1 |
| 13 | `rename` | EBX = from, ECX = to | 0 / -1 |

## Security: attack ↔ defence

| Attack | Breaks | Defence | Called from |
|---|---|---|---|
| `bad_filename` | trust | `filename_check` | `vfs_lookup`, `ramdisk_create`, `ramdisk_move` |
| `file_offset_wrap` | consistency | `file_range` | `ramdisk_write` |
| `delete_open_file` | consistency | `busy_check` | `ramdisk_delete` |
| `dir_cycle` | consistency | `no_cycle` | `ramdisk_move` |

## Known limits

- Only one driver (the RAM disk): no real disk yet.
- No permissions on files: every process may read and write everything.
- A file is at most 64 KiB; a directory holds at most 64 entries.
