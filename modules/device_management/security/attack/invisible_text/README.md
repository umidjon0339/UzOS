# Attack: invisible_text

| | |
|---|---|
| **Stopped by** | [`contrast_check`](../../defence/contrast_check/) |
| **Breaks** | trust: what the screen shows is what happened |
| **Severity** | medium |

## What it does

It installs a colour theme that hides text. Text cells only store a
colour **number**; the theme decides what that number looks like. Make
colour 15 ("white") the same as the background and every message
printed in white is still on screen, but unreadable.

| Theme | Hides |
|---|---|
| `evil-all`: all 16 colours = background | everything |
| `evil-white`: only colour 15 = background | exactly the normal messages |

The second is the sneaky one: the screen looks normal, it just never
shows the important lines.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the trick |
| `live` | calls the real `palette_use()` with both themes; both must be refused and the current theme must stay |

## Uses

`palette_use()` and `palette_current()` from vga/palette.

## Try it

```
seclab attack invisible_text live
theme                             # still the theme you had
```
