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
├── commands/     fs_commands.h/.c + one folder per command, each with cmd_<name>.h/.c:
│                 ls cd pwd mkdir touch rm cat write cp mv wc grep hexdump
│                 stat umask truncate   rmdir tree find du realpath basename dirname
│                 ln readlink unlink
│                 fscalls/ (+ meta.s dirs.s links.s)   ring 3 programs call every fs syscall
├── syscalls/     one folder each (number + .h + .c):
│                 mkdir/ unlink/ chdir/ getcwd/ rename/
│                 rmdir/ mkdirat/ unlinkat/ renameat/ renameat2/ fchdir/
│                 getdents/ getdents64/                            reading a directory
│                 stat/ lstat/ fstat/ fstatat/ statx/              what a file is
│                 access/ faccessat/ faccessat2/                   may I?
│                 chmod/ fchmod/ fchmodat/ chown/ fchown/ lchown/ fchownat/ umask/
│                 truncate/ ftruncate/ fallocate/                  its size
│                 utime/ utimes/ utimensat/                        its times
│                 link/ linkat/ symlink/ symlinkat/ readlink/ readlinkat/   more names
└── security/     <attack>/  atk_<attack>.c + README + defence/<defence>/
                  (16 pairs, the same shape as cpu_ and process_management)
```

Four kinds of folders and nothing else: `commands/`, `syscalls/`,
`security/`, `core/`. Each command, syscall, attack and defence folder
holds what it needs; `core/` keeps only what the kernel calls itself or
state that must exist once (see [core/README.md](core/README.md)).
Every `#include` names the full path, with what it takes from it.

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

## The inode: metadata

Every node carries what `stat` reports (Linux's "inode"):

| Field | Set when |
|---|---|
| `ino` | created: a number never reused (`vfs_new_ino`) |
| `nlink` | names pointing at it: 1, +1 per hard link (at most 16) |
| `atime` | the file is read |
| `mtime` | its bytes change (write, truncate); a directory's entries change |
| `ctime` | ANYTHING changes: bytes, mode, owner, name |
| `dev` | 1 = the RAM disk, 2 = `/proc` |

The clock is `vfs_now()`: the RTC read once, then counted on with the
timer. `vfs_stat()` turns a node into Linux's `struct stat`: UzOS's two
mode digits (owner, others) show as three (group = others), and a
directory you may read also shows `x` (you may enter it).

A new file's mode is `rw/rw` minus the creator's **umask**
(`process_t.umask`, inherited, default `022`) and never writable by
others, whatever the umask says (`secure_umask`).

## Directories as descriptors

`open(dir, O_READ | O_DIRECTORY)` (io_management) gives a descriptor
for a directory, read-only. It is what the `*at` calls start from
(`vfs_start_dir`), what `fchdir` enters, and what `getdents64` lists:
its offset counts entries (0 `.`, 1 `..`, then the directory's own).
Every listing re-checks read permission (`dir_read_check`), and every
directory a path passes through needs read (`search_check`), the dirfd
too.

Trees have limits: a full path never passes 255 bytes (`path_length`),
and nothing sits deeper than 32 levels (`depth_limit`), so `tree`,
`find`, `du` and `rm -r` (which recurse, with one shared path buffer)
always fit their kernel stack. `unlink` removes files only, `rmdir`
empty directories, as in Linux.

## Links: more than one name

```
 /home/a ──┐                      a HARD link is a second directory entry
           ├──► inode 7 "abc"     for the same inode: same bytes, owner,
 /home/b ──┘    nlink 2           times. Deleting one name leaves the other.

 /home/s ──► "a"  (a SYMBOLIC link: a tiny file holding a path,
                   followed when the path is walked)
```

- A hard link is an entry node whose `target` is the inode; `vfs_inode()`
  turns any entry into its inode. `vfs_lookup` returns the inode,
  `vfs_lookup_nofollow` stops at a symlink (`lstat`, `readlink`,
  `lchown`), `vfs_lookup_entry` gives the name itself (`rename`, `rm`).
- A walk follows at most 8 links (`link_depth`: ELOOP), and in a
  directory everyone may write, only links that belong to the follower
  or the directory owner (`protected_symlinks`).
- You may hard-link only your own files, never a directory
  (`protected_hardlinks`); a file has at most 16 names (`nlink_bounds`).
- Deleting the last name of an open file keeps the inode until the last
  descriptor closes (`inode_refcount`), as in Linux.

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
| `stat <path>...` | inode, links, owner, rights (0644/-rw-r--r--), the three times | `sys_stat` |
| `umask [022]` | show / set the bits new files leave out | `sys_umask` |
| `truncate -s SIZE <path>` | cut or extend with zeros (`4K`, `1M`) | `sys_truncate` (size_cap) |
| `touch [-r ref] <path>` | create, or set times to now / to another file's | `sys_utimensat` (time_owner) |
| `rm [-r] <path>...` | a file; an empty directory; with `-r` a whole tree | `sys_unlink`, `sys_rmdir` |
| `rmdir <dir>...` | empty directories only | `sys_rmdir` |
| `tree [path]` | draw the tree | `vfs_readdir`, depth_limit |
| `find [path] [-name x] [-type f\|d]` | search a tree | `vfs_readdir`, depth_limit |
| `du [-s] [path]` | bytes per directory | `vfs_readdir` |
| `realpath`, `basename`, `dirname` | where a path leads; its last name; the rest | `vfs_lookup`, text |
| `ln [-s] <target> <name>` | a hard link; with `-s` a symbolic one | `sys_link`, `sys_symlink` |
| `readlink <link>` | where a symbolic link points | `sys_readlink` |
| `unlink <name>` | remove one name (the file lives while it has others) | `sys_unlink` |
| `fscalls [meta\|dirs\|links]` | ring 3 programs call every filesystem syscall; each answer checked (meta 43, dirs 28, links 27) | usertask, `.s` in its folder |

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
| 97–101 | `stat`, `lstat`, `fstat`, `fstatat`, `statx` | path or fd, buffer | 0 / -1 |
| 102–104 | `access`, `faccessat`, `faccessat2` | path, R_OK 4 / W_OK 2 / X_OK 1 (= read) | 0 / -1 |
| 105–107 | `chmod`, `fchmod`, `fchmodat` | path or fd, Linux mode (0644) | 0 / -1 (owner or root) |
| 108–111 | `chown`, `fchown`, `lchown`, `fchownat` | path or fd, uid, gid (ignored) | 0 / -1 (CAP_CHOWN) |
| 112 | `umask` | Linux mask (022) | the old mask |
| 113–115 | `truncate`, `ftruncate`, `fallocate` | path or fd, size | 0 / -1 (size_cap) |
| 116–118 | `utime`, `utimes`, `utimensat` | path, times (0 = now) | 0 / -1 (time_owner) |
| 119 | `rmdir` | path | 0 / -1 (busy_check: empty only) |
| 120–121 | `mkdirat`, `unlinkat` | dirfd, path, mode / AT_REMOVEDIR | 0 / -1 |
| 122–123 | `renameat`, `renameat2` | olddirfd, old, newdirfd, new (, RENAME_NOREPLACE / EXCHANGE) | 0 / -1 |
| 124 | `fchdir` | dirfd | 0 / -1 (search_check) |
| 125–126 | `getdents`, `getdents64` | dirfd, buffer, size | bytes, 0 at the end, -1 (dir_read_check) |
| 127–128 | `link`, `linkat` | old, new (`linkat`: dirfds, AT_SYMLINK_FOLLOW 0x400) | 0 / -1 (protected_hardlinks, nlink_bounds) |
| 129–130 | `symlink`, `symlinkat` | target text, new name | 0 / -1 |
| 131–132 | `readlink`, `readlinkat` | path, buffer, size | bytes (no `\0`) / -1 if not a link |

The `*at` calls take a directory descriptor first: `AT_FDCWD` (-100)
means the current directory; an absolute path ignores it.

## Security: attack ↔ defence

| Attack | Breaks | Defence | Called from |
|---|---|---|---|
| `bad_filename` | trust | `filename_check` | `vfs_lookup`, `ramdisk_create`, `ramdisk_move` |
| `file_offset_wrap` | consistency | `file_range` | `ramdisk_write` |
| `delete_open_file` | consistency | `busy_check` | `ramdisk_delete` |
| `dir_cycle` | consistency | `no_cycle` | `ramdisk_move` |
| `umask_weak` | integrity | `secure_umask` | `ramdisk_create` |
| `huge_file` | resource | `size_cap` | `truncate`, `ftruncate`, `fallocate` |
| `timestamp_forgery` | integrity | `time_owner` | `utime`, `utimes`, `utimensat` |
| `dirent_leak` | disclosure | `dir_read_check` | `getdents`, `getdents64` |
| `long_path` | resource | `path_length` | `ramdisk_create`, `ramdisk_move`, `ramdisk_exchange` |
| `deep_nesting` | liveness | `depth_limit` | the same; tree walkers stop at 32 |
| `path_walk_bypass` | privilege | `search_check` | `vfs_lookup` (every directory), `fchdir` |
| `symlink_race` | privilege | `protected_symlinks` | `vfs_lookup` (every link followed) |
| `symlink_loop` | liveness | `link_depth` | the same: at most 8 hops |
| `hardlink_abuse` | privilege | `protected_hardlinks` | `link`, `linkat` |
| `nlink_overflow` | consistency | `nlink_bounds` | `ramdisk_link` |
| `use_after_unlink` | consistency | `inode_refcount` | `vfs_close`, `ramdisk_delete` |

## Known limits

- Only one driver (the RAM disk): no real disk yet.
- A file is at most 64 KiB; a directory holds at most 64 entries.
