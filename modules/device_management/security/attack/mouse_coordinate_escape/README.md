# Attack: mouse coordinate escape

**Breaks:** `consistency` · **Severity:** MEDIUM · **Paired defence:** [`mouse_bounds`](../../defence/mouse_bounds/README.md)

Decoded mouse movement can propose negative or off-screen coordinates. The
demo probes all screen edges without changing the pointer state or
interacting with mouse hardware.

```text
seclab attack mouse_coordinate_escape
seclab attack mouse_coordinate_escape live
seclab verify mouse_bounds
```
