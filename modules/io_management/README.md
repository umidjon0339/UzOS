# io_management — file descriptors: open, read, write, close

Programs never get pointers to files or devices. They get a small
number, a **file descriptor**, and pass it to `read` and `write`. The
kernel looks the number up in the calling process's table:

```
process_t.fd_table          fd   points to              opened as
                            0    keyboard  (stdin)      read
                            1    screen    (stdout)     write
                            2    screen    (stderr)     write
open("notes", O_READ) ->    3    VFS node "notes"       read, offset 0
```

The same `read`/`write` works for the keyboard, the screen and files.
That is the Unix idea "everything is a file".

```
io_management/
├── core/fd/      fd.h/.c           hand out / take back descriptor numbers
├── syscalls/     open/ read/ write/ close/ seek/   (number + .h + .c)
├── commands/     io_commands.h/.c + ioinfo/
└── security/     attack/<id>/  defence/<id>/   (3 pairs, each with a README)
```

## One read() from start to end

```
read(3, buf, 100)
  descriptor_bounds: is 3 a real, open descriptor opened for reading?
  transfer_bounds:   is buf a sane buffer, 100 <= 16 KiB?
  node = fd_table[3]
  vfs_read(node, fd_offset[3], 100, buf)     -> filesystem_management
  fd_offset[3] += bytes read
```

From ring 3 there is one more check, before all of this: the `int 0x80`
handler makes sure `buf` is the program's own **mapped** memory
(`user_ptr_bounds`, memory_management).

Permissions are checked once, at `open()`: reading needs read
permission on the file, writing needs write permission, creating needs
write permission on the directory (user_management `file_access`). A
descriptor you already hold keeps working.

## open() flags

| Flag | Meaning |
|---|---|
| `O_READ` / `O_WRITE` | what you want to do (at least one) |
| `O_CREATE` | create the file if it does not exist |
| `O_TRUNC` | start from an empty file |
| `O_APPEND` | every write goes to the end |

## System calls

| # | Syscall | Arguments | Result |
|---|---|---|---|
| 14 | `open` | EBX = path, ECX = flags | descriptor / -1 |
| 15 | `read` | EBX = fd, ECX = buffer, EDX = count | bytes read / -1 |
| 16 | `write` | EBX = fd, ECX = buffer, EDX = count | bytes written / -1 |
| 17 | `close` | EBX = fd | 0 / -1 |
| 18 | `seek` | EBX = fd, ECX = offset, EDX = SEEK_SET/CUR/END | new position / -1 |

## Commands

| Command | What it does | Uses |
|---|---|---|
| `ioinfo [pid]` | a process's descriptors: mode, offset, file path | `fd_table`, `vfs_path_of` |

`cat`, `write`, `cp`, `wc`, `grep` and `hexdump` (filesystem_management)
all work through these syscalls.

## Security: attack ↔ defence

| Attack | Breaks | Defence | Called from |
|---|---|---|---|
| `invalid_descriptor` | trust | `descriptor_bounds` | `sys_read`, `sys_write`, `sys_close`, `sys_seek` |
| `oversized_transfer` | trust | `transfer_bounds` | `sys_read`, `sys_write` |
| `fd_leak` | resource | `fd_cleanup` | the reaper, when a process ends |
