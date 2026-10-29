# Defence: canvas_clip

| | |
|---|---|
| **Stops** | [`canvas_overflow`](../../attack/canvas_overflow/) |
| **Protects** | isolation: a drawing stays inside its own pixel buffer |
| **Called from** | `canvas_fill_rect()` and `canvas_blit()` in [gui/canvas](../../../gui/canvas/canvas.c) |

## The rule

Before drawing, cut the rectangle to the canvas:

```
left = max(x, 0)          right  = min(x + w, width)
top  = max(y, 0)          bottom = min(y + h, height)
nothing left?  -> draw nothing
```

`x + w` is computed in 64 bits, so huge values cannot wrap around.

## Why

A pixel lives at `pixels[y * width + x]`. The old window manager's text
drawing only checked the right and bottom edges:

```c
if (x + cx < win->width && y + cy < win->height)   // no check for < 0!
    win->buffer[(y + cy) * win->width + (x + cx)] = color;
```

With `x = -8` that writes 8 pixels before the row, and for row 0 that is
**before the whole buffer**, inside whatever heap block comes first.
Windows dragged half off-screen hit this all the time.

Now every rectangle goes through one clip function, and single pixels
are bounds-checked in `canvas_put_pixel()`.

## Uses

- `canvas_t` from gui/canvas

## Try it

```
seclab attack canvas_overflow live
seclab verify canvas_clip
```
