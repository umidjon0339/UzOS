# Attack: setuid_escalation

| | |
|---|---|
| **Stopped by** | [`setuid_guard`](../../defence/setuid_guard/) |
| **Breaks** | privilege: a normal user stays a normal user |
| **Severity** | critical |

## What it does

The ring 3 program [setuid_escalation.s](setuid_escalation.s) runs as
guest (uid 1), calls `setuid(0)`, then `getuid()`, and leaves both
answers in the usertask mailbox.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains what uid 0 would give the program |
| `live` | runs it; expected: `setuid(0) returned -1, getuid() is now 1` |

## Uses

- `usertask_start()` / `usertask_mailbox()` from process_management/core/usertask
- syscalls 24 `setuid` and 23 `getuid`

## Try it

```
su
seclab attack setuid_escalation live
```
