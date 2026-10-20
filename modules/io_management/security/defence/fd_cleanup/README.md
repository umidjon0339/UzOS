# Defence: fd_cleanup

| | |
|---|---|
| **Stops** | [`fd_leak`](../../attack/fd_leak/) |
| **Protects** | resource: open counts and descriptors come back |
| **Called from** | the reaper (`def_reap_zombies` in process_management), before a dead process's slot is freed |

## The rule

When a process ends, every descriptor it still has open is closed.

## Why

Each open descriptor holds its file's `open_count` up by one, and
`busy_check` refuses to delete a file while that count is above zero.
A program that ends without calling `close()` (because it forgot,
crashed, or was killed) would hold the count up **forever**: nobody
could ever delete that file again.

Real kernels do the same: when a process exits, the kernel closes all
its files for it.

## Uses

- `fd_release()` / `fd_count()` from core/fd
- `sec_log()` from the security lab: a leak that had to be cleaned up is evidence

## Try it

```
seclab attack fd_leak live
seclab log
```
