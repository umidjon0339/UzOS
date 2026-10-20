# Defence: descriptor_bounds

| | |
|---|---|
| **Stops** | [`invalid_descriptor`](../../attack/invalid_descriptor/) |
| **Protects** | trust: descriptor numbers from a program are checked |
| **Called from** | `sys_read`, `sys_write`, `sys_close`, `sys_seek` in [syscalls/](../../../syscalls/) |

## The rules

1. `0 <= fd < 16`: the descriptor indexes a 16-entry table
2. `stdin` (0) can only be read; `stdout`/`stderr` (1, 2) only written
3. a file descriptor must be open, and opened for what you are doing:
   no `write()` on a file opened with `O_READ`

## Why

A descriptor is just a number the program passes in, and the kernel
uses it as an array index:

```c
vfs_node_t *file = self->fd_table[fd];     // fd = -1 or 9999?
```

Out of range, that reads a random word next to the table and treats it
as a file pointer. A closed slot holds `NULL`. Rule 3 makes "read-only"
mean something.

## Uses

- `process_t.fd_table` / `fd_flags` (process_management), `O_READ` /
  `O_WRITE` from syscalls/open
- `sec_log()` from the security lab

## Try it

```
seclab attack invalid_descriptor live
seclab log
```
