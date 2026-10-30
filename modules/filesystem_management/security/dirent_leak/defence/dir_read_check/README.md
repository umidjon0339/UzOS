# Defence: dir_read_check

| | |
|---|---|
| **Stops** | [`dirent_leak`](../../) |
| **Protects** | disclosure: directory entries |
| **Called from** | `sys_getdents`, `sys_getdents64` |

## The rule

Listing a directory needs read permission on it **now**, at every
`getdents`, not only when the descriptor was opened (Linux checks only
at open).

## Try it

```
seclab verify dir_read_check
```
