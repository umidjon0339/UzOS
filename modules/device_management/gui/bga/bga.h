#ifndef BGA_H
#define BGA_H

/*
 * bga.h - Bochs Graphics Adapter: the video card QEMU emulates.
 *
 * It is controlled through two I/O ports: write a register number to
 * the INDEX port, then its value to the DATA port. Once a graphics mode
 * is on, every pixel is a 32-bit 0x00RRGGBB value in the "linear
 * framebuffer", a plain block of memory at FRAMEBUFFER_START.
 */

#include <stdint.h>

// Is a BGA card there? (Reads its ID register.)
int bga_is_available(void);

// Switch to a graphics mode, e.g. 1024 x 768 x 32 bits. The card takes
// any size up to 2560 x 1600. Returns 0 if the mode was refused.
int bga_set_video_mode(uint16_t width, uint16_t height, uint16_t bits_per_pixel);

// Switch graphics off again: the card goes back to VGA text mode.
void bga_disable(void);

// Where the visible pixels live in memory.
uint32_t *bga_framebuffer(void);

#endif
