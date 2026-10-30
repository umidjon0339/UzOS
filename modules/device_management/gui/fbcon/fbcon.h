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
 *   display          characters    (one per screen size)
 *   800x600          100 x 37
 *   1024x768         128 x 48
 *   1280x720         160 x 45
 *   1600x900         200 x 56
 *   1920x1080        240 x 67
 *
 * Cells with background colour 0 are "transparent": the wallpaper (a
 * gradient from the theme) shows through behind the text.
 */

#include "display.h"

extern const display_t fbcon_800x600;
extern const display_t fbcon_1024x768;
extern const display_t fbcon_1280x720;
extern const display_t fbcon_1600x900;
extern const display_t fbcon_1920x1080;

#endif
