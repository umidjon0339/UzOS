# Defence: vm_access_check

| | |
|---|---|
| **Stops** | [`memory_peek`](../../) |
| **Protects** | disclosure: other processes' memory, and the kernel's |
| **Called from** | `sys_process_vm_readv`, `sys_process_vm_writev`, `sys_ptrace` (PEEKDATA / POKEDATA) |

## The rule

1. Never a system process.
2. Ptrace rights: the caller itself, its own user's **dumpable**
   process, or `CAP_PTRACE`.
3. The range is mapped **user** memory (`user_ptr_bounds`): kernel
   addresses are never "remote memory".

## Try it

```
seclab verify vm_access_check
seclab attack memory_peek live
```
