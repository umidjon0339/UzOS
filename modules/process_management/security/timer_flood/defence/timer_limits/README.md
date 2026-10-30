# Defence: timer_limits

| | |
|---|---|
| **Stops** | [`timer_flood`](../../) |
| **Protects** | resource: the work done on every timer tick |
| **Called from** | `sys_setitimer()` in [syscalls/setitimer](../../../../syscalls/setitimer/) |

## The rule

A user may have at most **8 periodic** interval timers armed in all
their processes together (`CAP_RESOURCE`: no limit). A one-shot timer is
always allowed. No interval is shorter than one tick (`sleep_bounds`
rounds up), so a timer fires at most 100 times a second.

## Try it

```
seclab verify timer_limits
seclab attack timer_flood live
```
