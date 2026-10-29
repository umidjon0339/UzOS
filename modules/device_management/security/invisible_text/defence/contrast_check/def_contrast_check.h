#ifndef DEF_CONTRAST_CHECK_H
#define DEF_CONTRAST_CHECK_H

#include <stdint.h>

// How different two colours' brightness must be (0-255 scale).
#define MIN_CONTRAST 48

// Brightness of 0x00RRGGBB as the eye sees it: 0 (black) - 255 (white).
int def_brightness(uint32_t rgb);

// Is text in 'fg' readable on 'bg'? 1 = yes, 0 = refused (logged with 'what').
int def_contrast_ok(uint32_t fg, uint32_t bg, const char *what);

#endif
