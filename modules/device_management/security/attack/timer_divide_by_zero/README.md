# Attack: invalid timer frequency

**Breaks:** `resource` · **Severity:** CRITICAL · **Paired defence:** [`timer_frequency`](../../defence/timer_frequency/README.md)

A zero frequency divides by zero during PIT setup. Frequencies below the
representable divisor range also produce a divisor too large for the PIT's
16-bit counter. The demo probes these boundaries; its live run submits only
the zero-frequency case, which must be refused before hardware I/O.

```text
seclab attack timer_divide_by_zero
seclab attack timer_divide_by_zero live
seclab verify timer_frequency
```
