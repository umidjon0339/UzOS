# Attack: timer_divide_by_zero

| | |
|---|---|
| **Stopped by** | [`timer_frequency`](../../defence/timer_frequency/) |
| **Breaks** | trust: numbers are checked before use |
| **Severity** | critical |

## What it does

`init_timer(hz)` computes `divisor = 1193180 / hz` and writes it to the
PIT's 16-bit counter.

| `init_timer(...)` | Effect without the check |
|---|---|
| `0` | division by zero: CPU exception 0, the kernel halts |
| `10` | divisor 119318 is cut to 16 bits: the timer runs at 22 Hz |
| `2000000` | divisor 0, which the PIT reads as 65536: 18 Hz |

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains each bad rate |
| `live` | calls the real `init_timer()` with all three, then checks the timer still runs at 100 Hz |

## Uses

`init_timer()` and `timer_frequency()` from timer/.

## Try it

```
seclab attack timer_divide_by_zero live
uptime
```
