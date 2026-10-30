# Defence: time_owner

| | |
|---|---|
| **Stops** | [`timestamp_forgery`](../../) |
| **Protects** | integrity: a file's history |
| **Called from** | `sys_utime`, `sys_utimes`, `sys_utimensat` |

## The rule

1. **Chosen** times: the owner, or `CAP_FILES` (root).
2. Times set to **now**: also anyone who may write the file.
3. `ctime` is never chosen: it always becomes now.

These are Linux's rules.

## Try it

```
seclab verify time_owner
```
