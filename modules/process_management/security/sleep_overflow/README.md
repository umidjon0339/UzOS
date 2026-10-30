# Attack: sleep_overflow

| | |
|---|---|
| **Stopped by** | [`sleep_bounds`](defence/sleep_bounds/) |
| **Breaks** | liveness: a wait lasts as long as it was asked to |
| **Severity** | low |

## What it does

Times from a program become timer ticks. In 32-bit arithmetic big ones
wrap: `nanosleep(42949673 s)` becomes 0.3 s, `sleep(0xFFFFFFFF ms)`
becomes 0. A protective timeout fires at once; a "far future" deadline
is already past. Malformed times (`nsec >= 1e9`, negative) are nonsense.

## In UzOS

`simulate` shows what 32-bit math makes of them; `live` feeds them to
`sleep_bounds` as `nanosleep`, `setitimer` and `sleep()` do.
`proccalls timers` sends the bad ones from ring 3: refused.

## Try it

```
seclab attack sleep_overflow
seclab attack sleep_overflow live
```
