#ifndef FBCON_H
#define FBCON_H

/*
 * fbcon.h - The graphics console: text drawn pixel by pixel.
 *
 * In graphics mode the card no longer knows about characters: it shows
 * whatever 32-bit pixels are in the framebuffer. So for every cell we
 * draw the character's 8x16 glyph ourselves, in the theme's real RGB
 * colours. 1024 / 8 = 128 columns, 768 / 16 = 48 rows.
 *
 * Cells with background colour 0 are "transparent": the wallpaper (a
 * gradient from the theme) shows through behind the text.
 */

#include "display.h"

extern const display_t fbcon_1024x768;

#endif
