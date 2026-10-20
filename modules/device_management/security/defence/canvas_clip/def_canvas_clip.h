#ifndef DEF_CANVAS_CLIP_H
#define DEF_CANVAS_CLIP_H

#include "canvas.h"

// Cut the rectangle (*x, *y, *w, *h) down to the part inside canvas 'c'.
// Returns 0 if nothing of it is inside (draw nothing).
int def_canvas_clip(const canvas_t *c, int *x, int *y, int *w, int *h);

#endif
