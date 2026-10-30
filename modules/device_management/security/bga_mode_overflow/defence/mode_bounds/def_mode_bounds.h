#ifndef DEF_MODE_BOUNDS_H
#define DEF_MODE_BOUNDS_H

#include <stdint.h>
#include "memory_layout.h"

// The largest video mode UzOS will program on the BGA card.
#define BGA_MAX_WIDTH   2560
#define BGA_MAX_HEIGHT  1600

// ...and that largest mode must fit the video memory the VMM maps, or
// drawing its last rows would write past the mapping (a page fault).
// Checked by the compiler: the build fails if someone breaks it.
_Static_assert(BGA_MAX_WIDTH * BGA_MAX_HEIGHT * 4 <= FRAMEBUFFER_END - FRAMEBUFFER_START,
               "the biggest BGA mode must fit the mapped framebuffer");

// Can the card be set to 'width' x 'height' at 'bpp'? Both sizes must be
// non-zero and within the maximum, and bpp a real depth (15/16/24/32).
// bga_set_video_mode() asks this before it programs the card, so the
// framebuffer size (width * height * bpp/8) cannot overflow or be zero.
// 1 = ok, 0 = refused (logged).
int def_bga_mode_ok(uint16_t width, uint16_t height, uint16_t bpp);

#endif
