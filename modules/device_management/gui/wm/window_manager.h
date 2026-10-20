#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

/*
 * window_manager.h - Windows you can click and drag.
 *
 * Every window owns a canvas. Programs draw into it with the canvas_*
 * functions; each frame the window manager paints the desktop, then the
 * windows from back to front (the "z-order"), then the mouse pointer.
 */

#include "canvas.h"
#include "mouse.h"

#define MAX_WINDOWS      8
#define TITLE_BAR_HEIGHT 20
#define BORDER           2

void wm_init(void);

// A new window on top of the others. Returns its id, or -1.
int wm_create_window(int x, int y, int width, int height, const char *title);

// Close one window / all windows and free their pixels.
void wm_destroy_window(int id);
void wm_destroy_all(void);

// The window's drawing surface (NULL for a bad id).
canvas_t *wm_window_canvas(int id);

// React to the mouse: raise the clicked window, drag by the title bar.
void wm_handle_mouse(const mouse_state_t *mouse);

// Paint desktop, windows and pointer onto 'screen'.
void wm_draw(canvas_t *screen, const mouse_state_t *mouse);

#endif
