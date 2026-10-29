# device_management/security — attacks and their defences

Every pair lives in its own folder named for the **attack**, with the
**defence** nested inside it — the same clean shape as cpu_management:

```
security/<attack>/atk_<attack>.c
security/<attack>/README.md
security/<attack>/defence/<defence>/def_<defence>.c / .h
security/<attack>/defence/<defence>/README.md
```

`seclab list` shows them all; `seclab attack <id> [live]` runs one;
`seclab verify <defence>|all` runs the selftests. Each defence is the
exact check the **driver** makes at the point the attack strikes, so a
defence is live code, not a demo.

## The 12 pairs

| Attack | Defence | The driver that calls it |
|---|---|---|
| `timer_divide_by_zero` | `timer_frequency` | `timer/timer.c` rejects a PIT rate that divides badly |
| `mouse_coordinate_escape` | `mouse_bounds` | `mouse/mouse.c` clamps the cursor to the screen |
| `mouse_desync` | `mouse_packet_sync` | `mouse/mouse.c` drops a packet whose sync bit is wrong |
| `irq_storm` | `irq_mask` | `pic/pic.c` masks a line that fires too fast |
| `canvas_overflow` | `canvas_clip` | `gui/canvas/canvas.c` clips every draw to the canvas |
| `rtc_garbage` | `rtc_validate` | `rtc/rtc.c` rejects out-of-range clock fields |
| `invisible_text` | `contrast_check` | `vga/palette/palette.c` refuses a theme with no contrast |
| `keyboard_flood` | `key_queue_bound` | `keyboard/keyboard.c` drops keys when the ring is full |
| `speaker_abuse` | `speaker_freq_guard` | `speaker/speaker.c` bounds the PC-speaker frequency |
| `bga_mode_overflow` | `mode_bounds` | `gui/bga/bga.c` rejects an out-of-range video mode |
| `window_exhaustion` | `window_quota` | `gui/wm/window_manager.c` caps open windows at MAX_WINDOWS |
| `window_size_overflow` | `window_bounds` | `gui/wm/window_manager.c` bounds a window before kmalloc |

The last five were added after the first seven. `bga_mode_overflow` and
`window_size_overflow` closed **real gaps** — their driver had no size
check until the attack revealed it; the rest verify guards that already
existed. `keyboard_flood` has no audit log (its check runs in the
keyboard interrupt, where logging is unsafe), so its verdict comes from
the selftest rather than a refusal count.

The pairs are registered in
[security_management/core/catalog/catalog.c](../../security_management/core/catalog/catalog.c);
the shared `attack_t` / `defence_t` types are in
[security_management/core/registry](../../security_management/core/registry/).
