# Defence: mouse_bounds

| | |
|---|---|
| **Stops** | [`mouse_coordinate_escape`](../../attack/mouse_coordinate_escape/) |
| **Protects** | consistency: the pointer is always a valid screen position |
| **Called from** | the packet decoder in [mouse/](../../../mouse/mouse.c), for every move |

## The rule

```
0 <= x <= 1023        0 <= y <= 767
```

Anything outside is moved to the nearest edge ("clamped").

## Why

The mouse only reports **movement** (up to ±255 per packet), not a
position. The driver adds it up. Keep moving left at `x = 0` and the sum
goes negative. The GUI uses `x` and `y` as pixel indexes, so a negative
or too-large position means drawing outside the screen buffer.

Hitting the screen edge is completely normal, so clamping is **not**
logged as an attack. `seclab` judges this defence by its selftest.

## Uses

- `MOUSE_SCREEN_MAX_X` / `MOUSE_SCREEN_MAX_Y` from mouse/

## Try it

```
seclab attack mouse_coordinate_escape live
seclab verify mouse_bounds
```
