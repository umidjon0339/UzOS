#include "window_manager.h"
#include "gui.h"
#include "mouse.h"
#include "kheap.h"
#include "string.h"
#include "font8x8.h"

static window_t windows[MAX_WINDOWS];
static window_t *z_order[MAX_WINDOWS];
static int num_windows = 0;

static int prev_mouse_left = 0;

void wm_init(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].is_active = 0;
    }
    num_windows = 0;
    prev_mouse_left = 0;
}

int wm_create_window(int x, int y, int width, int height, const char *title) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].is_active) {
            windows[i].id = i;
            windows[i].x = x;
            windows[i].y = y;
            windows[i].width = width;
            windows[i].height = height;
            
            int t_len = strlen(title);
            if (t_len > 63) t_len = 63;
            memcpy(windows[i].title, title, t_len);
            windows[i].title[t_len] = '\0';
            
            windows[i].buffer = (uint32_t *)kmalloc(width * height * 4);
            if (!windows[i].buffer) return -1;
            
            // Fill background white
            for (int j = 0; j < width * height; j++) {
                windows[i].buffer[j] = GUI_COLOR_WHITE;
            }
            
            windows[i].is_active = 1;
            windows[i].is_dragging = 0;
            
            z_order[num_windows] = &windows[i];
            num_windows++;
            return i;
        }
    }
    return -1;
}

static void wm_bring_to_front(window_t *win) {
    int idx = -1;
    for (int i = 0; i < num_windows; i++) {
        if (z_order[i] == win) {
            idx = i;
            break;
        }
    }
    if (idx == -1 || idx == num_windows - 1) return;
    
    // Shift others down
    for (int i = idx; i < num_windows - 1; i++) {
        z_order[i] = z_order[i + 1];
    }
    z_order[num_windows - 1] = win;
}

void wm_draw_rect(int win_id, int x, int y, int width, int height, uint32_t color) {
    if (win_id < 0 || win_id >= MAX_WINDOWS || !windows[win_id].is_active) return;
    window_t *win = &windows[win_id];
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (x + j >= 0 && x + j < win->width && y + i >= 0 && y + i < win->height) {
                win->buffer[(y + i) * win->width + (x + j)] = color;
            }
        }
    }
}

void wm_draw_string(int win_id, int x, int y, const char *str, uint32_t color, uint32_t bg_color) {
    if (win_id < 0 || win_id >= MAX_WINDOWS || !windows[win_id].is_active) return;
    window_t *win = &windows[win_id];
    
    int curr_x = x;
    int curr_y = y;
    while (*str) {
        if (*str == '\n') {
            curr_x = x;
            curr_y += 10;
        } else {
            unsigned char c = (unsigned char)*str;
            if (c < 128) {
                char *bitmap = font8x8_basic[c];
                for (int cy = 0; cy < 8; cy++) {
                    for (int cx = 0; cx < 8; cx++) {
                        if ((bitmap[cy] >> cx) & 1) {
                            if (curr_x + cx < win->width && curr_y + cy < win->height)
                                win->buffer[(curr_y + cy) * win->width + (curr_x + cx)] = color;
                        } else if (bg_color != 0x01010101) {
                            if (curr_x + cx < win->width && curr_y + cy < win->height)
                                win->buffer[(curr_y + cy) * win->width + (curr_x + cx)] = bg_color;
                        }
                    }
                }
            }
            curr_x += 8;
        }
        str++;
    }
}

void wm_update(void) {
    // Check mouse clicks (top to bottom)
    int clicked_window = 0;
    
    if (mouse_b_left) {
        if (!prev_mouse_left) {
            // Mouse just went down
            for (int i = num_windows - 1; i >= 0; i--) {
                window_t *win = z_order[i];
                // Check if click is in title bar (y-20 to y)
                if (mouse_x >= win->x && mouse_x <= win->x + win->width &&
                    mouse_y >= win->y - 20 && mouse_y <= win->y) {
                    
                    wm_bring_to_front(win);
                    win->is_dragging = 1;
                    win->drag_offset_x = mouse_x - win->x;
                    win->drag_offset_y = mouse_y - win->y;
                    clicked_window = 1;
                    break;
                }
                
                // Click in window body
                if (mouse_x >= win->x && mouse_x <= win->x + win->width &&
                    mouse_y >= win->y && mouse_y <= win->y + win->height) {
                    wm_bring_to_front(win);
                    clicked_window = 1;
                    break;
                }
            }
        } else {
            // Mouse is held down
            if (num_windows > 0) {
                window_t *top_win = z_order[num_windows - 1];
                if (top_win->is_dragging) {
                    top_win->x = mouse_x - top_win->drag_offset_x;
                    top_win->y = mouse_y - top_win->drag_offset_y;
                }
            }
        }
    } else {
        // Mouse released
        if (num_windows > 0 && z_order[num_windows - 1]->is_dragging) {
            z_order[num_windows - 1]->is_dragging = 0;
        }
    }
    
    prev_mouse_left = mouse_b_left;
    
    // Draw Desktop
    gui_clear_screen(GUI_COLOR_BG);
    
    // Draw Windows (bottom to top)
    for (int i = 0; i < num_windows; i++) {
        window_t *win = z_order[i];
        
        // Draw frame / title bar
        uint32_t border_col = (i == num_windows - 1) ? GUI_COLOR_BLUE : GUI_COLOR_GREY;
        gui_draw_rect(win->x - 2, win->y - 22, win->width + 4, win->height + 24, GUI_COLOR_GREY);
        gui_draw_rect(win->x, win->y - 20, win->width, 20, border_col);
        gui_draw_string(win->x + 5, win->y - 14, win->title, GUI_COLOR_WHITE, border_col);
        
        // Blit window buffer
        for (int row = 0; row < win->height; row++) {
            for (int col = 0; col < win->width; col++) {
                gui_draw_pixel(win->x + col, win->y + row, win->buffer[row * win->width + col]);
            }
        }
    }
    
    // Draw Mouse
    gui_draw_rect(mouse_x, mouse_y, 2, 12, GUI_COLOR_WHITE);
    gui_draw_rect(mouse_x, mouse_y, 8, 2, GUI_COLOR_WHITE);
    gui_draw_pixel(mouse_x, mouse_y, GUI_COLOR_RED);
    gui_draw_pixel(mouse_x+1, mouse_y+1, GUI_COLOR_RED);
    
    gui_swap_buffers();
}
