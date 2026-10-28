# device_management — drivers: timer, keyboard, mouse, clock, screen, GUI

A **driver** is the code that talks to one piece of hardware. Most
devices get the CPU's attention with an **IRQ** (interrupt request); the
PIC turns that into an interrupt vector, and `arch/x86/isr` calls the
driver's handler.

```
device_management/
├── pic/       pic.h/.c            interrupt controller: IRQ -> vector, masks, EOI
├── timer/     timer.h/.c          PIT, IRQ 0: ticks, uptime, CPU accounting, drives the scheduler
├── keyboard/  keyboard.h/.c       PS/2 keyboard, IRQ 1: scancodes -> key queue, Ctrl+C
├── mouse/     mouse.h/.c          PS/2 mouse, IRQ 12: packets -> pointer + buttons
├── rtc/       rtc.h/.c            real-time clock (CMOS): date and time
├── speaker/   speaker.h/.c        PC speaker: tones through PIT channel 2
├── font/      font.h/.c           the active font (16- and 8-row glyphs)
│              font_data.c         the UzOS font tables (generated, see OFL.txt)
│              glyphs.h/.c         UzOS's own icons, drawn as readable pictures
├── vga/       vga.h/.c            the console: text, colours, scroll-back, status row
│   │          display.h           what a display must do (an interface)
│   ├── text/    vga_text.h/.c     display: VGA text mode, 80x25 and 80x50
│   └── palette/ palette.h/.c      colour themes, programmed into the VGA DAC
├── gui/
│   ├── fbcon/   fbcon.h/.c        display: the 1024x768 graphics console
│   ├── bga/     bga.h/.c          the video card: graphics mode on/off
│   ├── canvas/  canvas.h/.c       draw rectangles/text/copies, always clipped
│   │            font8x8.h/.c      8x8 pixel font for the desktop
│   ├── screen/  screen.h/.c       back buffer + present (double buffering)
│   └── wm/      window_manager.h/.c  windows, z-order, drag
├── commands/  device_commands.h/.c + date/ devices/ startx/ beep/ play/
│                                    theme/ font/ mode/
├── syscalls/  uptime/ time/       milliseconds since boot; seconds since 1970
└── security/  attack/<id>/  defence/<id>/   (7 pairs, each with a README)
```

## From key press to shell

```
key pressed -> keyboard raises IRQ 1 -> PIC -> vector 33
  -> arch/x86/isr: count it, send EOI, call keyboard_callback()
  -> read scancode from port 0x60 -> character -> key queue
kernel_main loop: keyboard_pop_key() -> shell_handle_key()
```

Shift, Caps Lock and Ctrl are held keys: the driver remembers them.
With Ctrl down, C does not type `c`: the driver calls the handler the
shell installed (`keyboard_on_ctrl_c`), because Ctrl+C must work even
while nobody reads the queue. If the handler does not take it, it is
queued as `KEY_CTRL_C` (ASCII 3, as on every terminal). Other Ctrl
combinations type nothing.

## IRQ lines

| IRQ | Vector | Device | Driver |
|---|---|---|---|
| 0 | 32 | PIT timer | `timer/` |
| 1 | 33 | PS/2 keyboard | `keyboard/` |
| 2 | 34 | cascade to the second PIC | `pic/` |
| 12 | 44 | PS/2 mouse | `mouse/` |

`pic_remap()` starts with **every** line masked. Each driver unmasks
its own line once its handler is registered (see `irq_mask`). The
`devices` command shows the lines and how often each fired.

## The console in layers

```
vga_print_str("hi")     every module prints through the console
  vga.c                 history of 256 lines, cursor, colours, status row,
                        full-screen mode for snake / top / startx
    display_t           an interface: put(x, y, cell), cursor(...), ...
      text/             VGA text mode: write the cell to 0xB8000, the card
                        draws it with the font in its plane 2
      gui/fbcon/        graphics: draw the cell's 8x16 glyph pixel by pixel
    font/               the glyphs both displays draw
    palette/            what the 16 colour numbers really look like
```

`mode` switches between the three displays without losing the text:

| Mode | Characters | How |
|---|---|---|
| `80x25` | 80 × 25 | text mode, 16-row font |
| `80x50` | 80 × 50 | text mode, 8-row font (same 400 pixel lines) |
| `1024x768` | 128 × 48 | graphics mode, wallpaper behind the text |

A **cell** is 2 bytes: the character (code page 437) and its colour
(low 4 bits text, high 4 bits background). Colours are only numbers; a
**theme** (`theme`) loads 16 real RGB colours into the card's DAC, so
every character on screen changes colour at once.

The **font** (`font`) is the UzOS font, based on Terminus (SIL Open
Font License, see [font/OFL.txt](font/OFL.txt)), or the card's own BIOS
font. Both get the UzOS icons ([font/glyphs.c](font/glyphs.c)) at codes
the BIOS used for old symbols: `GLYPH_FOLDER`, `GLYPH_CHECK`,
`GLYPH_CRESCENT`...

## The real-time clock

The PIT timer only counts ticks since boot. The **RTC** is a separate
chip with a battery that knows the date. Its registers are read through
ports `0x70`/`0x71`, usually in BCD (`0x59` = 59). The driver waits
until the chip is not updating, reads twice until both reads agree, and
checks the result (`rtc_validate`). `make run` starts QEMU with
`-rtc base=localtime` so the clock shows your local time.

## The GUI in layers

```
startx (commands/)     reads mouse + keys, refreshes windows, ~30 frames/s
  window_manager       windows, z-order, dragging, draws the desktop
    canvas             rectangles, text, copies: clipped to the buffer
      screen           back buffer -> framebuffer (no flicker)
        bga            video card: 1024x768x32 on, off
```

ESC leaves the desktop. Graphics overwrites the text font and the
card's text settings, so `vga_restore_display()` opens the console's
display again (text mode: every register, the font and the theme), and
the console redraws its text from history.

## Commands and syscall

The `uptime` COMMAND moved to [cpu_management](../cpu_management/README.md#commands)
(it shows the load average); the `uptime` SYSCALL stays here.

| Command | What it does | Uses |
|---|---|---|
| `date` | date and time, and the Unix time | `rtc_*` |
| `devices` | IRQ lines: vector, masked/open, handler, count; hardware found | `isr_*`, `pic_get_mask` |
| `startx` | graphical desktop with a live System Monitor; ESC returns | `gui/*`, `mouse`, `keyboard` |
| `beep [hz]` | a 200 ms tone | `speaker_beep` |
| `play` | a short melody | `speaker_beep`, `timer_wait` |
| `theme [name]` | list the colour themes, or switch | `palette_*`, `vga_reload` |
| `font [uzos\|bios]` | list the fonts with a sample, or switch | `font_*`, `vga_reload` |
| `mode [name]` | list the displays, or switch | `vga_set_display` |

| # | Syscall | Result (EAX) |
|---|---|---|
| 8 | `uptime` | milliseconds since boot |
| 19 | `time` | seconds since 1970-01-01 (Unix time), 0 if the clock is unreadable |

## Security: attack ↔ defence

| Attack | Breaks | Defence | Called from |
|---|---|---|---|
| `timer_divide_by_zero` | trust | `timer_frequency` | `init_timer` |
| `mouse_coordinate_escape` | consistency | `mouse_bounds` | mouse packet decoder |
| `mouse_desync` | consistency | `mouse_packet_sync` | mouse packet decoder |
| `irq_storm` | liveness | `irq_mask` | `pic_unmask` |
| `canvas_overflow` | isolation | `canvas_clip` | `canvas_fill_rect`, `canvas_blit` |
| `rtc_garbage` | trust | `rtc_validate` | `rtc_decode` |
| `invisible_text` | trust | `contrast_check` | `palette_use`, the shell's `color` |

The mouse attacks feed crafted bytes into the **real** packet decoder
with interrupts off, then put the pointer back where it was.
