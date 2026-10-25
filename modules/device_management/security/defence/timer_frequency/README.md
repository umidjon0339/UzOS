# Defence: timer_frequency

| | |
|---|---|
| **Stops** | [`timer_divide_by_zero`](../../attack/timer_divide_by_zero/) |
| **Protects** | trust: a number is checked before it is divided by |
| **Called from** | `init_timer()` in [timer/](../../../timer/timer.c) |

## The rule

```
19 <= hz <= 1,193,180
```

## Why

The PIT counts down from a 16-bit **divisor** at 1,193,180 Hz:

```c
divisor = 1193180 / hz;
```

| `hz` | divisor | result |
|---|---|---|
| `0` | division by zero | CPU exception 0: the kernel halts |
| `10` | 119318, more than 16 bits | cut to 53782: runs at 22 Hz |
| `2000000` | 0 | the PIT reads 0 as 65536: runs at 18 Hz |

A wrong tick rate makes `sleep`, `uptime` and the scheduler's time
slices all wrong, without any error message.

## Uses

- `PIT_INPUT_HZ` / `TIMER_HZ` from timer/
- `audit_log()` from security_management (core/audit)

## Try it

```
seclab attack timer_divide_by_zero live
uptime                           # still 100 per second
```
