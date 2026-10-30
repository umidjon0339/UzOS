# Defence: handler_bounds

| | |
|---|---|
| **Stops** | [`handler_hijack`](../../) |
| **Protects** | integrity: SIGKILL and SIGSTOP always work; handlers stay in user space |
| **Called from** | `sys_rt_sigaction()` in [syscalls/rt_sigaction](../../../../syscalls/rt_sigaction/) |

## The rule

1. SIGKILL and SIGSTOP cannot be caught or ignored.
2. A handler is `SIG_DFL`, `SIG_IGN`, or a user-space address.
3. With `SA_RESTORER`, the restorer is a user-space address.

## Why

SIGKILL and SIGSTOP are how root and the shell always regain control.
A handler or restorer address is where the kernel later sends the CPU;
it must never be the kernel's own code.

## Try it

```
seclab verify handler_bounds
seclab attack handler_hijack live
```
