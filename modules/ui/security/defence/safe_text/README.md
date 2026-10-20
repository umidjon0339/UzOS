# Defence: safe_text

| | |
|---|---|
| **Stops** | [`output_spoof`](../../attack/output_spoof/) |
| **Protects** | consistency: a table shows exactly what is in the kernel |
| **Called from** | `ui_cell()` in [core/box](../../../core/box/box.c), which tables use for every name; `top` calls it directly |

## The rule

```
every character < 32, and 127        ->  shown as '?'   (logged)
text longer than its column          ->  cut, last character '»'   (normal, not logged)
```

## Why

A table trusts that each cell is one short piece of text. But a name is
just bytes, and some bytes are commands to the screen:

| Byte | What the console does |
|---|---|
| `\n` (10) | starts a new line |
| `\r` (13) | goes back to the start of the line, over what is there |
| `\b` (8) | moves back one character |

A process named `"x\n  1  RUNNING  0  init"` makes `ps` print a
**second row** that looks exactly like a real process. Escape codes
(`\033[2J` on other terminals) can even clear the screen. A very long
name pushes the next columns out of place.

So names go through one gate before any table prints them, and come out
as exactly one cell of exactly the right width.

## Uses

- `sec_log()` from the security lab

## Try it

```
seclab attack output_spoof live
seclab log
```
