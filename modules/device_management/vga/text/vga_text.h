#ifndef VGA_TEXT_H
#define VGA_TEXT_H

/*
 * vga_text.h - VGA text mode: the card draws the characters for us.
 *
 * Video memory at 0xB8000 holds 2 bytes per cell (character, colour).
 * The card reads it 70 times a second and draws each character with the
 * font stored in its own memory (plane 2). Two layouts:
 *
 *   80x25   16-pixel-tall font    the classic PC screen
 *   80x50    8-pixel-tall font    the same 400 pixel lines, twice the text
 */

#include "display.h"

extern const display_t vga_text_80x25;
extern const display_t vga_text_80x50;

#endif
