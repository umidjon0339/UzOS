# Attack: mouse_coordinate_escape

| | |
|---|---|
| **Stopped by** | [`mouse_bounds`](../../defence/mouse_bounds/) |
| **Breaks** | consistency: the pointer is a real screen position |
| **Severity** | medium |

## What it does

A PS/2 mouse never says where it is, only how far it moved (at most 255
per packet). The driver keeps a running sum. Pushing left at `x = 0`
makes the sum negative; the GUI then uses it as a pixel index and draws
outside the screen buffer.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows where 6 big moves left would end without a check |
| `live` | feeds 6 × 255 moves in each of the 4 directions to the **real** packet decoder (interrupts off), prints the pointer after each, then puts it back |

## Uses

`mouse_handle_byte()`, `mouse_get_state()`, `mouse_set_state()` from mouse/.

## Try it

```
seclab attack mouse_coordinate_escape live
```
