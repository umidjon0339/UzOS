# Defence: irq_mask

| | |
|---|---|
| **Stops** | [`irq_storm`](../../attack/irq_storm/) |
| **Protects** | liveness: no device can keep the CPU busy forever |
| **Called from** | `pic_unmask()` in [pic/](../../../pic/pic.c) |

## The rule

An IRQ line may only be unmasked (let through) if it is the cascade line
(IRQ 2), or a driver has registered a handler for it.

`pic_remap()` starts with **every** line masked. The timer, keyboard and
mouse drivers each unmask their own line once their handler is in place.

## Why

The old code unmasked all 16 lines at boot. A line with no driver can
still fire, for example a serial port receiving noise. Nobody reads the
device, so it keeps its request up. The CPU stops, finds no handler,
sends EOI, and is interrupted again at once. This is an **interrupt
storm**: the CPU spends all its time on interrupts nobody wants.

Opening a line only when someone handles it follows the same idea as
user permissions: everything is closed unless there is a reason.

## Uses

- `isr_has_handler()` from arch/x86/isr
- `sec_log()` from the security lab

## Try it

```
devices                          # PIC column: only used lines are "open"
seclab attack irq_storm live
```
