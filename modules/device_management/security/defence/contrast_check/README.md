# Defence: contrast_check

| | |
|---|---|
| **Stops** | [`invisible_text`](../../attack/invisible_text/) |
| **Protects** | trust: what the screen shows is what happened |
| **Called from** | `palette_use()` in [vga/palette](../../../vga/palette/palette.c) for every theme, and the shell's [`color`](../../../../shell/commands/color/cmd_color.c) command |

## The rule

```
brightness(c) = (299 R + 587 G + 114 B) / 1000        0 = black, 255 = white
| brightness(text) - brightness(background) | >= 48
```

The weights are how bright the eye sees each channel: green looks much
brighter than red, and red brighter than blue.

For a theme, the normal text colours (7 and 15) are checked against the
background (colour 0). For `color`, the chosen pair is checked, using
the real colours of the current theme.

## Why

Text in the same colour as its background is printed, but nobody can
read it. That can hide exactly what the user needs to see:

- a warning, or "process 3 killed"
- a fake `password:` prompt, while the real one is invisible

A screen that hides things lies by omission.

| Example | Brightness difference | Result |
|---|---|---|
| white on black | 255 | allowed |
| classic grey `AAAAAA` on black | 170 | allowed |
| dark blue `0000AA` on black | 19 | refused |
| black on black | 0 | refused |

## Uses

- `palette_rgb()` from vga/palette
- `audit_log()` from security_management (core/audit)

## Try it

```
seclab attack invisible_text live
color black black                 # refused
color blue                        # in the classic theme: refused
```
