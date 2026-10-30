# Defence: link_depth

| | |
|---|---|
| **Stops** | [`symlink_loop`](../../) |
| **Protects** | liveness: every path walk ends |
| **Called from** | `vfs_lookup()` (core/vfs), before every link it follows |

## The rule

One path walk follows at most **8** symbolic links; the next fails with ELOOP. (Linux: 40, at most 8 nested.)

## Try it

```
seclab verify link_depth
```
