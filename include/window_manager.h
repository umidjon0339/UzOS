#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <stdint.h>

#define MAX_WINDOWS 16

typedef struct {
    int id;
    int x;
    int y;
    int width;
    int height;
    char title[64];
    uint32_t *buffer; // The internal drawing canvas of the window
    int is_active;
    
    // Internal WM state
    int is_dragging;
    int drag_offset_x;
    int drag_offset_y;
} window_t;

void wm_init(void);
int wm_create_window(int x, int y, int width, int height, const char *title);
void wm_destroy_window(int id);

// Drawing API for window contents
void wm_draw_rect(int win_id, int x, int y, int width, int height, uint32_t color);
void wm_draw_string(int win_id, int x, int y, const char *str, uint32_t color, uint32_t bg_color);

// Desktop update loop
void wm_update(void);

#endif
