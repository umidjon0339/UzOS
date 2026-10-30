# Defence: session_check

| | |
|---|---|
| **Stops** | [`pgid_hijack`](../../) |
| **Protects** | isolation: groups stay inside their session |
| **Called from** | `sys_setpgid()`, `sys_setsid()` |

## The rule (POSIX)

1. You may move yourself or your own child, nobody else.
2. Only within your session; a session leader never changes group.
3. Into a new group (pgid = the target's pid), or one that already
   exists in the same session.
4. `setsid()` only if you are not a group leader already.

## Try it

```
seclab verify session_check
seclab attack pgid_hijack live
proccalls session
```
