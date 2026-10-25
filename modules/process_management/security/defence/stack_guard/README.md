# Defence: stack_guard

| | |
|---|---|
| **Stops** | [`stack_overflow`](../../attack/stack_overflow/) |
| **Protects** | isolation: a process stays inside its own memory |
| **Called from** | `scheduler_yield()` in [core/scheduler](../../../core/scheduler/scheduler.c) |

## The rule

```
stack_base <= ESP <= stack_base + 4096
```

The scheduler runs on the current process's stack, so it reads the real
`ESP` register and asks this guard before every switch. If `ESP` is
outside, the process becomes a zombie and never runs again.
`kernel_main` runs on the boot stack (`stack_base = 0`) and is skipped.

## Why

Each process has a 4 KiB stack from the heap. Deep recursion keeps
pushing below the bottom, and the CPU does not stop it: it silently
overwrites whatever the heap put there. Silent corruption is worse than
a crash, because nothing reports it.

This guard is a software check after the fact. The hardware fix is a
**guard page**: leave the page below each stack unmapped, so the first
bad push page-faults at once. That needs per-process paging.

## Uses

- the real `ESP`, read by the scheduler
- `process_t.stack_base`
- `audit_log()` from security_management (core/audit)

## Try it

```
seclab verify stack_guard
seclab attack stack_overflow live
```
