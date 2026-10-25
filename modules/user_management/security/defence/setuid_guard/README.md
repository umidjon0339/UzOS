# Defence: setuid_guard

| | |
|---|---|
| **Stops** | [`setuid_escalation`](../../attack/setuid_escalation/) |
| **Protects** | privilege: a normal user cannot give itself root's rights |
| **Called from** | `sys_setuid()` in [syscalls/setuid](../../../syscalls/setuid/sys_setuid.c) |

## The rule

```
1. the wanted uid belongs to a real user
2. CAP_SETUID (root) may become any user   (to give up its rights)
3. anyone else may only stay itself
```

## Why

Every other check in UzOS trusts the uid: `kill_guard`, `slot_quota`,
`priority_bounds`, `file_access` all ask `caps_has(uid, ...)`, and uid 0
holds every capability. If a program could set its own uid to 0, all
of them would let it through. `setuid` is only for going
**down** (root to a normal user), never up.

## Uses

- `users_find_uid()` from [core/users](../../../core/users/)
- `caps_has(uid, CAP_SETUID)` from security_management [core/caps](../../../../security_management/core/caps/)
- `audit_log()` from security_management (core/audit)

## Try it

```
su
seclab attack setuid_escalation live
seclab verify setuid_guard
```
