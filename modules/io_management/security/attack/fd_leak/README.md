# Attack: fd_leak

| | |
|---|---|
| **Stopped by** | [`fd_cleanup`](../../defence/fd_cleanup/) |
| **Breaks** | resource: open counts come back down |
| **Severity** | medium |

## What it does

Every open descriptor keeps its file's `open_count` up, and a file with
a count above zero cannot be deleted. A process that **ends without
`close()`** (it forgot, crashed, or was killed) leaves that count up
forever. The file is stuck until reboot.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the leak |
| `live` | creates `/seclab_leak`, starts a process that opens it and returns at once, waits for it to end and be reaped, then shows the open count and tries `rm` |

With `fd_cleanup`, the count is back to 0 and the file is deleted.

## Uses

`process_create()` (process_management), `sys_open()` from syscalls/,
`ramdisk_create()` / `sys_unlink()` (filesystem_management), `timer_wait()`.

## Try it

```
seclab attack fd_leak live
audit                            # fd_cleanup closed the leaked file
```
