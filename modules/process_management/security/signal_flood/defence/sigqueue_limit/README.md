# Defence: sigqueue_limit

| | |
|---|---|
| **Stops** | [`signal_flood`](../../) |
| **Protects** | resource: the queued-signal pool (64 entries for everyone) |
| **Called from** | `sigqueue_add()` in [core/signal/sigqueue.c](../../../../core/signal/sigqueue.c) |

## The rule

A user may have at most **min(RLIMIT_SIGPENDING, 16)** queued signals
waiting at a time. Root too: raising the rlimit does not go past 16.

## Why

A queued signal holds kernel memory until it is delivered, and a target
that blocks the signal never takes it. Without a share per user, one
user takes the whole pool. Linux has the same limit (`ulimit -i`).

## Uses

- `RLIMIT_SIGPENDING` of the sender (cpu_management core/rlimit)
- `audit_log()` from security_management, to record each refusal

## Try it

```
seclab verify sigqueue_limit
seclab attack signal_flood live
```
