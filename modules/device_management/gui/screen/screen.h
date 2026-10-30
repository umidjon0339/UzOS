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

#define SCREEN_BPP    32

// Switch to graphics at 'width' x 'height' (e.g. 1024 x 768). Returns 0
// (and stays in text mode) if there is no BGA card, the size is refused,
// or there is no RAM for the back buffer.
int screen_open(int width, int height);

// Change the size while open (a new back buffer). Returns 0 if the new
// size did not work; the old size is then kept.
int screen_resize(int width, int height);

int screen_width(void);
int screen_height(void);

// Back to text mode; frees the back buffer.
void screen_close(void);

canvas_t *screen_canvas(void);      // draw the next frame here
void screen_present(void);          // show it

#endif
