# Attack: canvas_overflow

| | |
|---|---|
| **Stopped by** | [`canvas_clip`](../../defence/canvas_clip/) |
| **Breaks** | isolation: a drawing stays inside its own pixels |
| **Severity** | high |

## What it does

Pixel `(x, y)` is stored at `pixels[y * width + x]`. If `x` or `y` is
negative, or a shape runs past the right or bottom edge, that index
points **outside** the buffer, into whatever the heap put next to it.
The old window manager's text drawing checked only the right and bottom
edges, so text at a negative `x` wrote in front of the window's buffer.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | shows the index the old code computed for `x = -8` |
| `live` | allocates a 16 × 16 canvas with 64 guard words (`0xDEADBEEF`) on each side, draws 5 shapes hanging over every edge (including one where `x + w` overflows a 32-bit int), and counts damaged guard words. Then frees it. |

## Uses

`canvas_fill_rect()`, `canvas_draw_string()`, `canvas_blit()` from
gui/canvas, `kmalloc()` / `kfree()` from memory_management.

## Try it

```
seclab attack canvas_overflow live
```
