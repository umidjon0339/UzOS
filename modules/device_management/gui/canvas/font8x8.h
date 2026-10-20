#ifndef FONT8X8_H
#define FONT8X8_H

/*
 * font8x8.h - An 8x8 pixel bitmap font for ASCII 0-127.
 *
 * Each character is 8 bytes, one per row, top to bottom. In each byte,
 * bit 0 is the LEFTMOST pixel. Example, 'A' (0x41):
 *
 *   0x0C  ..##....
 *   0x1E  .####...
 *   0x33  ##..##..
 *   ...
 *
 * Font by Daniel Hepper, based on Marcel Sondaar's. Public domain.
 */

#include <stdint.h>

#define FONT_WIDTH  8
#define FONT_HEIGHT 8

extern const uint8_t font8x8_basic[128][8];

#endif
