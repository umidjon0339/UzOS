# Defence: pidfd_handle

| | |
|---|---|
| **Stops** | [`pid_reuse_race`](../../) |
| **Protects** | identity: a handle names one process for ever |
| **Called from** | `sys_pidfd_open`, `sys_pidfd_getfd`; every pidfd use goes through `pidfd_target()` (core/pidfd → pcb_generation) |

## The rule

1. A pidfd is made only for a **live** process (not a zombie, not a system process).
2. It never moves: once its process ends, every use fails (`ESRCH`).
3. `pidfd_getfd` copies another process's open file: it needs **ptrace
   rights** (your own dumpable process, or `CAP_PTRACE`).

## Try it

```
seclab verify pidfd_handle
seclab attack pid_reuse_race live
proccalls wait
```
