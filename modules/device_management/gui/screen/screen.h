#ifndef SCREEN_H
#define SCREEN_H

/*
 * screen.h - The graphics screen, with double buffering.
 *
 * Drawing straight onto the visible framebuffer would flicker: you would
 * see each frame half drawn. So everything is drawn into a hidden "back
 * buffer" first, and screen_present() copies the finished frame to the
 * real framebuffer in one go.
 */

#include "canvas.h"

#define SCREEN_WIDTH  1024
#define SCREEN_HEIGHT 768
#define SCREEN_BPP    32

// Switch to graphics. Returns 0 (and stays in text mode) if there is no
// BGA card or no memory for the back buffer.
int screen_open(void);

// Back to text mode; frees the back buffer.
void screen_close(void);

canvas_t *screen_canvas(void);      // draw the next frame here
void screen_present(void);          // show it

#endif
