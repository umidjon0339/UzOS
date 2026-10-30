# Defence: ptrace_restrict

| | |
|---|---|
| **Stops** | [`ptrace_abuse`](../../) |
| **Protects** | disclosure: another process's registers and memory |
| **Called from** | `sys_ptrace()` (ATTACH, TRACEME), `strace -p` |

## The rule

1. Never a system process, never yourself.
2. Your own process, or any with `CAP_PTRACE`.
3. It must be dumpable (`prctl PR_SET_DUMPABLE`), unless `CAP_PTRACE`.
4. It is not traced already.
5. **Yama scope 1**: only an ancestor may attach, so a program cannot
   read your other programs just because the same user runs them;
   unless `CAP_PTRACE`.

## Try it

```
seclab verify ptrace_restrict
proccalls debug
```
