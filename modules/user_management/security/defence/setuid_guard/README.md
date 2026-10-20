# Defence: setuid_guard

| | |
|---|---|
| **Stops** | [`setuid_escalation`](../../attack/setuid_escalation/) |
| **Protects** | privilege: a normal user cannot give itself root's rights |
| **Called from** | `sys_setuid()` in [syscalls/setuid](../../../syscalls/setuid/sys_setuid.c) |

## The rule

```
1. the wanted uid belongs to a real user
2. root may become any user          (to give up its rights)
3. anyone else may only stay itself
```

## Why

Every other check in UzOS trusts the uid: `kill_guard`, `slot_quota`,
`priority_bounds`, `file_access`. If a program could set its own uid
to 0, all of them would let it through. `setuid` is only for going
**down** (root to a normal user), never up.

## Uses

- `users_find_uid()` from [core/users](../../../core/users/)
- `sec_log()` from the security lab

## Try it

```
su
seclab attack setuid_escalation live
seclab verify setuid_guard
```
