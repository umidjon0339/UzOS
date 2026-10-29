#ifndef DEF_WINDOW_BOUNDS_H
#define DEF_WINDOW_BOUNDS_H

// The largest window UzOS will allocate (also the screen's size).
#define WINDOW_MAX_WIDTH   1024
#define WINDOW_MAX_HEIGHT  768

// May a window of 'width' x 'height' be created? Both must be positive
// and within the screen. wm_create_window() asks this before it does
// kmalloc(width * height * 4), so that size can neither overflow nor
// drain the heap. 1 = ok, 0 = refused (logged).
int def_window_size_ok(int width, int height);

#endif
