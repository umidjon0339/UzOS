# Attack: oversized_transfer

| | |
|---|---|
| **Stopped by** | [`transfer_bounds`](../../defence/transfer_bounds/) |
| **Breaks** | trust: buffers are checked before copying |
| **Severity** | high |

## What it does

`read(fd, buf, count)` makes the kernel copy `count` bytes **into**
`buf`; `write` copies **from** it. The program chooses both numbers.

| Call | Problem without checks |
|---|---|
| `write(1, NULL, 5)` | copies from address 0 |
| `write(1, buf, 16 KiB + 1)` | one call larger than the kernel allows |
| `read(0, 0xFFFFFFF0, 32)` | the range wraps round to `0x10`: writes over low memory |

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists the bad buffers |
| `live` | really calls `sys_read()` / `sys_write()` with each, then a normal `write` that must still work |

## Uses

`sys_read()`, `sys_write()` from syscalls/.

## Try it

```
seclab attack oversized_transfer live
```
