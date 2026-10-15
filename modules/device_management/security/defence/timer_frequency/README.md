# Defence: timer frequency

**Counters:** [`timer_divide_by_zero`](../../attack/timer_divide_by_zero/README.md)

Before registering or programming the PIT, the driver accepts only
frequencies whose divisor fits the 16-bit counter. Invalid values are
refused and logged without touching hardware.

```text
seclab verify timer_frequency
seclab attack timer_divide_by_zero live
```
