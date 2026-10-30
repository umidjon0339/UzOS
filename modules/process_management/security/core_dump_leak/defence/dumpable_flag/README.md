# Defence: dumpable_flag

| | |
|---|---|
| **Stops** | [`core_dump_leak`](../../) |
| **Protects** | disclosure: a process's memory after it changed user |
| **Called from** | `sys_setuid()` (user_management), when the uid really changes |

## The rule

After a uid change the process is **not dumpable** (`prctl
PR_SET_DUMPABLE` may turn it back on). Every way into a process's
memory respects the flag: `ptrace` (ptrace_restrict),
`process_vm_readv`/`writev` (vm_access_check), `pidfd_getfd`
(pidfd_handle), `procinfo` / `pmap` / `/proc` (hidepid).

## Try it

```
seclab verify dumpable_flag
```
