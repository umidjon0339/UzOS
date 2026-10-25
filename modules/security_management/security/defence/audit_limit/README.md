# Defence: audit_limit

| | |
|---|---|
| **Stops** | [`log_flood`](../../attack/log_flood/) |
| **Protects** | integrity: one defence cannot wash the others out of the log |
| **Called from** | `audit_log()` in [core/audit](../../../core/audit/), for every event, before it goes into the ring |

## The rule

```
1. each defence may put 10 events into the ring per second
2. after that its events are still COUNTED, only not kept
3. a new second starts a new allowance
```

The first event it does not keep becomes one note in the ring:

```
audit_limit   flood of descriptor_bounds, the rest only counted
```

## Why

Without it, a flood of one cheap refusal fills all 32 slots and
pushes out the event that mattered. With it, a flood takes at most 10
slots a second, and `audit sum` still shows every refusal, kept or
not. Nothing is lost from the counters; only the ring is protected.

Linux does the same: `printk_ratelimit()` for kernel messages, and the
audit system has a `rate_limit` and counts what it drops (`lost`).

This defence cannot call `audit_log()` itself: it is part of the log.
`audit.c` writes its note instead.

## Uses

- `timer_ticks` (device_management) for the second

## Try it

```
seclab verify audit_limit
su
seclab attack log_flood live
audit sum                       # ONLY COUNTED column
```
