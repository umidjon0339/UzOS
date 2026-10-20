# Attack: irq_storm

| | |
|---|---|
| **Stopped by** | [`irq_mask`](../../defence/irq_mask/) |
| **Breaks** | liveness: no device keeps the CPU busy forever |
| **Severity** | medium |

## What it does

The PIC has 16 IRQ lines; a **mask** bit per line decides whether the
CPU hears it. The old `pic_remap()` opened all 16. A device on a line
with no driver, like a serial port picking up noise, raises its line and
nobody ever answers the device. The CPU is interrupted, finds no
handler, sends EOI, and is interrupted again: an **interrupt storm**.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists every open line and flags any without a handler |
| `live` | tries `pic_unmask()` on IRQ 3, 4, 5 and 7 (serial, sound, printer: no drivers), then checks the PIC mask did not change |

## Uses

`pic_unmask()`, `pic_mask()`, `pic_get_mask()` from pic/,
`isr_has_handler()` from arch/x86/isr.

## Try it

```
seclab attack irq_storm
seclab attack irq_storm live
devices
```
