# Attack: log_flood

| | |
|---|---|
| **Stopped by** | [`audit_limit`](../../defence/audit_limit/) |
| **Breaks** | integrity: the audit log still shows what happened |
| **Severity** | medium |

## What it does

The audit ring keeps the newest 32 events. Whoever wants a refusal
forgotten does not need to delete it: they only need 32 newer ones.

```
1. the thing to hide:  kill PID 1          -> kill_guard event #57
2. the flood:          close(-1) x 64      -> 64 descriptor_bounds events
3. `audit` now shows 32 close() refusals; #57 was written over
```

`close(-1)` costs nothing, needs no rights, and works from ring 3 as
well. Any user can do it.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains how many events it takes |
| `live` | does steps 1-2 for real, then searches the ring for event #57 by its sequence number |

Expected: `event #57: still in the log`, and `only counted, not kept: 54`.

## Uses

- `audit_snapshot()`, `audit_total()`, `audit_refused_by()` from [core/audit](../../../core/audit/)
- `process_kill()` (process_management), `sys_close()` (io_management)

## Try it

```
su
seclab attack log_flood live
audit
audit sum
```
