#ifndef DISPLAY_H
#define DISPLAY_H

/*
 * display.h - What the console needs from a screen.
 *
 * The console (vga.c) keeps the text: lines, colours, the cursor. It
 * does not care HOW a character gets onto the screen. A display does
 * that part. UzOS has two kinds:
 *
 *   vga/text/      text mode: write the character into video memory and
 *                  the card draws it (80x25 or 80x50)
 *   gui/fbcon/     graphics mode: draw every pixel of the character
 *                  ourselves (800x600 up to 1920x1080)
 *
 * This "table of function pointers" is how C does an interface: the
 * console calls display->put(...) and does not know which one it is.
 */

#include <stdint.h>

#define VGA_MAX_COLS 240        // the widest display: 1920 / 8
#define VGA_MAX_ROWS 67         // the tallest display: 1080 / 16

typedef struct {
    const char *name;           // "80x25" - what the `mode` command takes
    const char *about;          // one line for the `mode` list
    int cols, rows;
    int width, height;          // in pixels; 0 x 0 for a text mode

    int  (*open)(void);         // switch the hardware to this display; 0 = cannot
    void (*close)(void);        // before another display takes over
    void (*put)(int x, int y, uint16_t cell);       // char | colour << 8
    void (*cursor)(int x, int y, int visible);
    void (*reload)(void);       // the font or the theme changed
} display_t;

#endif
