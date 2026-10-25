# Defence: slot_quota

| | |
|---|---|
| **Stops** | [`fork_bomb`](../../attack/fork_bomb/) |
| **Protects** | resource: there are only 64 process slots |
| **Called from** | `process_create()` in [core/process](../../../core/process/process.c) |

## The rule

1. A normal user may own at most **8** processes.
2. The last **5** free slots are kept for root.

Whoever holds **CAP_RESOURCE** (root) skips both rules.

## Why

Without a cap, one user can take every slot and nothing else can start,
not even a shell to fix it. Rule 1 stops one user early. Rule 2 makes
sure root can always get in to clean up, even when the table is almost full.

## Uses

- `process_count_free_slots()` and the process table, to count slots
- `caps_has(uid, CAP_RESOURCE)` from security_management [core/caps](../../../../security_management/core/caps/)
- `audit_log()` from security_management (core/audit), to record each refusal

## Try it

```
seclab verify slot_quota
seclab attack fork_bomb live     # 8 created, then refused
audit
seclab repair
```
