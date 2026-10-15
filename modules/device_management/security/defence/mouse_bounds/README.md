# Defence: mouse bounds

**Counters:** [`mouse_coordinate_escape`](../../attack/mouse_coordinate_escape/README.md)

The shared clamp keeps mouse coordinates in the display's valid range. The
mouse IRQ uses it after applying each decoded packet, and its selftest covers
both screen edges and in-range coordinates.

```text
seclab verify mouse_bounds
seclab attack mouse_coordinate_escape live
```
