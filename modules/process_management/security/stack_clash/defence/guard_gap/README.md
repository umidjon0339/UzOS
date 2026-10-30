# Defence: guard_gap

| | |
|---|---|
| **Stops** | [`stack_clash`](../../) |
| **Protects** | isolation: what lies below a stack |
| **Called from** | `sys_execve()`: the segments before loading, the gap after mapping the stack |

## The rule

1. No program segment in the execve stack area (0xB0000000..0xB0400000).
2. The 8 pages (32 KiB) below a new stack are unmapped, so the CPU
   faults first. (Linux: `stack_guard_gap`, 1 MiB.)

## Try it

```
seclab verify guard_gap
```
