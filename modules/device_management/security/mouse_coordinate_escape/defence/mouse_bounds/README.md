# Defence: mouse_bounds

| | |
|---|---|
| **Stops** | [`mouse_coordinate_escape`](../../attack/mouse_coordinate_escape/) |
| **Protects** | consistency: the pointer is always a valid screen position |
| **Called from** | the packet decoder in [mouse/](../../../mouse/mouse.c), for every move |

## The rule

```
0 <= x <= width - 1        0 <= y <= height - 1
```

for the current screen (1023 and 767 at 1024 x 768). Anything outside is
moved to the nearest edge ("clamped"). When the screen gets smaller
(Ctrl+Down, `mode 800x600`) the pointer is clamped to the new edge at once.

## Why

The mouse only reports **movement** (up to ±255 per packet), not a
position. The driver adds it up. Keep moving left at `x = 0` and the sum
goes negative. The GUI uses `x` and `y` as pixel indexes, so a negative
or too-large position means drawing outside the screen buffer.

Hitting the screen edge is completely normal, so clamping is **not**
logged as an attack. `seclab` judges this defence by its selftest.

## Uses

- nothing: the mouse driver passes the current screen edge in (`max_x`, `max_y`, set by `mouse_set_screen()` whenever the size changes)

## Try it

```
seclab attack mouse_coordinate_escape live
seclab verify mouse_bounds
```
