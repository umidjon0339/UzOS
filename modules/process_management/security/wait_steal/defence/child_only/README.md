# Defence: child_only

| | |
|---|---|
| **Stops** | [`wait_steal`](../../) |
| **Protects** | privilege: exit statuses go to the parent only |
| **Called from** | `sys_wait4`, `sys_waitpid`, `sys_waitid` (a named pid); `exitlog_take` matches the parent too |

## The rule

A process may wait for, and collect, **its own children only**. Not
even root takes another parent's child.

## Try it

```
seclab verify child_only
seclab attack wait_steal live
proccalls wait
```
