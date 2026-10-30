# Defence: sleep_bounds

| | |
|---|---|
| **Stops** | [`sleep_overflow`](../../) |
| **Protects** | liveness: waits and timeouts mean what they say |
| **Called from** | `sys_sleep` (ms), `nanosleep`, `clock_nanosleep`, `setitimer`, `rt_sigtimedwait` |

## The rule

1. A negative time, or `nsec >= 1e9` (`usec >= 1e6`), is refused (EINVAL).
2. Conversion in 64 bits, rounded **up**: 1 ns still waits one tick.
3. Capped at 2^31 - 1 ticks (~248 days), so `deadline - now` fits a
   signed 32-bit number and comparisons never wrap.

## Try it

```
seclab verify sleep_bounds
proccalls timers
```
