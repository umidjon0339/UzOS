# Attack: rtc_garbage

| | |
|---|---|
| **Stopped by** | [`rtc_validate`](../../defence/rtc_validate/) |
| **Breaks** | trust: data from hardware is checked before use |
| **Severity** | medium |

## What it does

It plays a broken clock chip: register values that cannot be a date.

| Registers | Decoded without a check |
|---|---|
| all `0xFF` (no chip) | 2165-165-165 165:165:165 |
| month `0x13` | month 13 |
| day `0x30`, month `0x02` | 30 February |

The values go into the **real** decoder, `rtc_decode()`, the function
every date from the chip goes through.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows what `0xFF` decodes to and which table it would overrun |
| `live` | feeds all three register sets to `rtc_decode()`; each must be refused, then the real clock is read to show it still works |

Nothing is changed, so there is nothing to clean up.

## Uses

`rtc_decode()` and `rtc_read()` from rtc/.

## Try it

```
seclab attack rtc_garbage
seclab attack rtc_garbage live
```
