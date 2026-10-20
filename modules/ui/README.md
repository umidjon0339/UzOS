# ui — how UzOS looks

Every module prints things. This module makes them all look the same:
one set of colours by meaning, one way to say "error", one kind of
frame and bar. It also owns the things that are only about looks: the
boot log, the logo, the status bar and `uzfetch`.

```
ui/
├── core/
│   ├── style/      ui.h/.c         message kinds and colours: ui_error, ui_ok, ui_usage, ...
│   ├── box/        box.h/.c        frames ┌─┐, table cells (ui_cell), bars ███░░
│   ├── bootlog/    bootlog.h/.c    "[  OK  ] step  detail" lines while booting
│   ├── logo/       logo.h/.c       the UzOS banner and the Uzbek flag picture
│   └── statusbar/  statusbar.h/.c  the top bar, kept fresh by the statusd daemon
├── commands/       ui_commands.h/.c + uzfetch/ statusbar/
└── security/       attack/output_spoof  defence/safe_text
```

There is no `syscalls/` folder: programs print through `write` on
stdout (io_management); this module is about what the kernel prints.

## Colours by meaning

Commands never pick a raw colour for a message. They say what kind of
text it is:

| Kind | Looks like | Use for |
|---|---|---|
| `ui_error("rm", "...")` | ✗ rm: ... (red) | something failed |
| `ui_warn(...)` | ! ... (yellow) | worked, but watch out |
| `ui_ok(...)` | ✓ ... (green) | done |
| `ui_info(...)` | i ... (cyan) | for your information |
| `ui_usage("rm <path>")` | i usage: rm <path> | wrong arguments |
| `ui_title`, `ui_hint`, `ui_label` | headings, dim help, grey labels | |

The **normal text colour** is whatever `color` set. `ui_reset()` always
goes back to it, and the shell calls it after every command, so no
command can leave the screen in a strange colour.

The colours are numbers 0-15. What they really look like is decided by
the theme (`theme`, in device_management/vga/palette).

## Frames, cells and bars

```c
box_begin("Physical memory", 58);      ┌─ Physical memory ───────────┐
box_row();                             │ Usage   ███░░░░░░░░░   7%   │
ui_bar(used, total, 30);               └─────────────────────────────┘
box_row_end();
box_end();
```

The lines come from code page 437, the PC's character set: `0xDA` is ┌,
`0xC4` is ─, `0xDB` is █ and so on. `box_row_end()` pads the row with
spaces up to the right edge, so rows are always the same width.

`ui_cell(text, width)` prints exactly `width` columns. Tables use it for
every name they did not write themselves; it is where the `safe_text`
defence sits.

## The boot log

```
[ .... ] Physical memory          <- printed BEFORE the step runs
[  OK  ] Physical memory           127 MB RAM, 30340 frames free
```

`boot_begin()` prints the step first. If the step hangs, you still see
which one it was. When it ends, `'\r'` goes back to the start of the
line and `boot_ok()` writes the result over the dots.

## The status bar and its daemon

```
 ☾ UzOS │ /docs                         ⚙ 4 │ ▣ █░░░░ 7% │ ◷ 14:32:05
```

The console keeps row 0 for it (`vga_status_enable`). It is kept up to
date by **statusd**, a kernel process that loops forever:

```
build the row (clock, memory, processes, the shell's directory) -> show it -> sleep 500 ms
```

A process that runs in the background and that nobody calls is a
**daemon**. It is a system process (`kill` refuses it); `ps` shows it.
On the wider graphics console the bar also shows the uptime and date.

## Commands

| Command | What it does | Uses |
|---|---|---|
| `uzfetch` | the flag + a summary of the whole system | every module's counters, `cpuid` |
| `statusbar [on\|off]` | show or hide the top bar | core/statusbar |

## Security: attack ↔ defence

| Attack | Breaks | Defence | Called from |
|---|---|---|---|
| `output_spoof` | consistency | `safe_text` | `ui_cell()`, `top` |
