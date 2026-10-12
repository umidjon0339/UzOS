#include "gui.h"
#include "bga.h"
#include "kheap.h"
#define FONT8X8_IMPLEMENTATION
#include "font8x8.h"

static uint32_t *front_buffer = 0;
static uint32_t *back_buffer = 0;

static int screen_w = 1024;
static int screen_h = 768;

void gui_init(void) {
    bga_set_video_mode(screen_w, screen_h, 32);
    front_buffer = bga_get_framebuffer();
    
    // Allocate double buffer (1024 * 768 * 4 = 3MB)
    back_buffer = (uint32_t *)kmalloc(screen_w * screen_h * 4);
    
    if (!back_buffer) {
        // Fallback if allocation failed
        back_buffer = front_buffer;
    }
    
    gui_clear_screen(GUI_COLOR_BG);
    gui_swap_buffers();
}

void gui_draw_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= screen_w || y < 0 || y >= screen_h) return;
    back_buffer[y * screen_w + x] = color;
}

void gui_draw_rect(int x, int y, int width, int height, uint32_t color) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            gui_draw_pixel(x + j, y + i, color);
        }
    }
}

void gui_clear_screen(uint32_t color) {
    for (int i = 0; i < screen_w * screen_h; i++) {
        back_buffer[i] = color;
    }
}

void gui_draw_char(int x, int y, char c, uint32_t color, uint32_t bg_color) {
    unsigned char uc = (unsigned char)c;
    if (uc >= 128) return;
    
    char *bitmap = font8x8_basic[uc];
    for (int cy = 0; cy < 8; cy++) {
        for (int cx = 0; cx < 8; cx++) {
            if ((bitmap[cy] >> cx) & 1) {
                gui_draw_pixel(x + cx, y + cy, color);
            } else {
                // To support transparent backgrounds, only draw bg if it's not some specific transparent magic number.
                // For simplicity, we just draw it if it's not matching a dummy color, or we always draw.
                // Let's assume bg_color != 0x01010101 means opaque.
                if (bg_color != 0x01010101) {
                    gui_draw_pixel(x + cx, y + cy, bg_color);
                }
            }
        }
    }
}

void gui_draw_string(int x, int y, const char *str, uint32_t color, uint32_t bg_color) {
    int curr_x = x;
    int curr_y = y;
    while (*str) {
        if (*str == '\n') {
            curr_x = x;
            curr_y += 10;
        } else {
            gui_draw_char(curr_x, curr_y, *str, color, bg_color);
            curr_x += 8;
        }
        str++;
    }
}

void gui_swap_buffers(void) {
    if (back_buffer == front_buffer) return;
    
    // Fast memory copy
    for (int i = 0; i < screen_w * screen_h; i++) {
        front_buffer[i] = back_buffer[i];
    }
}
