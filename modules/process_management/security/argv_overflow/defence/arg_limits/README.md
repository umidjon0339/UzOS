# Defence: arg_limits

| | |
|---|---|
| **Stops** | [`argv_overflow`](../../) |
| **Protects** | resource: the kernel's copy buffer and the new program's stack |
| **Called from** | `sys_execve()`, for argv and then envp, before anything is copied |

## The rule

1. At most **32 strings** (argv + envp) and **2048 bytes** together (Linux: `ARG_MAX`, `E2BIG`).
2. From ring 3, every array slot and every string is the caller's own
   mapped memory (`user_ptr_bounds`).

## Try it

```
seclab verify arg_limits
```
