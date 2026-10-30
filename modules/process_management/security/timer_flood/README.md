# Attack: timer_flood

| | |
|---|---|
| **Stopped by** | [`timer_limits`](defence/timer_limits/) |
| **Breaks** | resource: the kernel's time on every tick is shared |
| **Severity** | low |

## What it does

Each process has three interval timers. Armed at the shortest interval,
each makes the kernel count, signal and deliver 100 times a second. A
user who arms all of them in every process they may have turns the
timer interrupt into their private worker.

## In UzOS

`live`: user 1 starts 4 `ticker` processes and tries to arm all 3
timers of each (12) at 10 ms, as `setitimer()` would. 8 are armed, the
rest refused. `repair` kills the tickers.

## Try it

```
seclab attack timer_flood live
procinfo <pid of a ticker>     # timers: real 10 ms (every 10 ms), ...
seclab repair
```
