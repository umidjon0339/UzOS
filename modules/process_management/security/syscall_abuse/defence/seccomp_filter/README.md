# Defence: seccomp_filter

| | |
|---|---|
| **Stops** | [`syscall_abuse`](../../) |
| **Protects** | privilege: a sandbox holds |
| **Called from** | the syscall handler, before **every** `int 0x80` of a sandboxed process |

## The rule

| Mode | Allowed | Anything else |
|---|---|---|
| strict | `read`, `write`, `exit`, `rt_sigreturn` | ends the program: SIGSYS |
| filter | the syscalls whose bit is set | the filter's action: SIGSYS, or -1 |

A filter only gets **stricter**: a new one is ANDed with the old, strict
stays strict, children inherit it. UzOS's filter is a list of allowed
syscalls (Linux runs a small BPF program to decide the same).

## Try it

```
seclab verify seccomp_filter
proccalls sandbox
```
