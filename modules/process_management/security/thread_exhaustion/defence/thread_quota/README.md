# Defence: thread_quota

| | |
|---|---|
| **Stops** | [`thread_exhaustion`](../../) |
| **Protects** | resource: process slots |
| **Called from** | `sys_clone()` and `sys_clone3()` (cpu_management), for `CLONE_THREAD` |

## The rule

A thread group (one program) has at most **4** live threads, whoever
runs it: root too. `slot_quota` limits a user; this limits a program.

## Try it

```
seclab verify thread_quota
seclab attack thread_exhaustion live
```
