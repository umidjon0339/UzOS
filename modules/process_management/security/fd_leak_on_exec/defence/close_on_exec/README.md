# Defence: close_on_exec

| | |
|---|---|
| **Stops** | [`fd_leak_on_exec`](../../) |
| **Protects** | disclosure: open files across execve |
| **Called from** | `sys_execve()`, right before the new program starts |

## The rule

1. Every descriptor opened with `O_CLOEXEC` is closed.
2. Every pidfd is closed too (Linux makes them close-on-exec).

The rest (0, 1, 2, plain files) stays: that is how a shell passes a
redirected file to a program.

## Try it

```
seclab verify close_on_exec
```
