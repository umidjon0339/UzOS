#ifndef GUI_H
#define GUI_H

#include <stdint.h>

#define GUI_COLOR_BLACK   0x000000
#define GUI_COLOR_WHITE   0xFFFFFF
#define GUI_COLOR_BLUE    0x0000FF
#define GUI_COLOR_GREEN   0x00FF00
#define GUI_COLOR_RED     0xFF0000
#define GUI_COLOR_GREY    0x888888
#define GUI_COLOR_BG      0x2A3441  // A nice dark blue-ish modern background

void gui_init(void);
void gui_draw_pixel(int x, int y, uint32_t color);
void gui_draw_rect(int x, int y, int width, int height, uint32_t color);
void gui_clear_screen(uint32_t color);
void gui_draw_char(int x, int y, char c, uint32_t color, uint32_t bg_color);
void gui_draw_string(int x, int y, const char *str, uint32_t color, uint32_t bg_color);
void gui_swap_buffers(void);

#endif
