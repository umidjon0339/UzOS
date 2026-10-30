# Defence: search_check

| | |
|---|---|
| **Stops** | [`path_walk_bypass`](../../) |
| **Protects** | privilege: private directories |
| **Called from** | `vfs_lookup()` / `vfs_lookup_parent()` (every directory passed, the first one too), `sys_fchdir()` |

## The rule

Passing **through** a directory needs read permission on it (UzOS has
no x bit: read means "search"). That covers `..` out of it, and a dirfd
opened before its mode changed.

## Try it

```
seclab verify search_check
```
