# Attack: invalid_descriptor

| | |
|---|---|
| **Stopped by** | [`descriptor_bounds`](../../defence/descriptor_bounds/) |
| **Breaks** | trust: descriptor numbers are checked |
| **Severity** | high |

## What it does

A file descriptor is just a number the program hands to the kernel,
which uses it as an index into the process's 16-entry `fd_table`.

| Call | Problem without checks |
|---|---|
| `read(-1)`, `read(16)` | reads a word outside the table as a "file" pointer |
| `read(9)`, never opened | the slot is `NULL` |
| `write(0)`, `read(1)` | stdin is input, stdout is output |
| `write()` on an `O_READ` file | "read-only" would mean nothing |
| `close(1)` | the screen is not a file |

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the bad uses |
| `live` | really makes each call (with a temporary read-only file for the last one), then deletes the file |

## Uses

`sys_open()`, `sys_read()`, `sys_write()`, `sys_close()` from syscalls/,
`sys_unlink()` from filesystem_management.

## Try it

```
seclab attack invalid_descriptor live
```
