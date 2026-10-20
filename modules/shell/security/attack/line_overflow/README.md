# Attack: line_overflow

| | |
|---|---|
| **Stopped by** | [`input_bound`](../../defence/input_bound/) |
| **Breaks** | isolation: typed text stays inside its buffer |
| **Severity** | high |

## What it does

The line you type into is a fixed array of 256 bytes. Holding a key
down sends hundreds of characters. Without a check, character 257 is
written **after** the array, over the next variable in memory. That is
the classic *buffer overflow*.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the overflow |
| `live` | builds a test line with 16 guard words (`0xDEADBEEF`) right behind it, sends 1000 keys through the real `line_insert()`, then checks: 255 stored, NUL at the end, guards intact |

The test uses its own line, so what you are typing is not touched.

## Uses

`line_t`, `line_clear()`, `line_insert()` from core/line.

## Try it

```
seclab attack line_overflow live
```
