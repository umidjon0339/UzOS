/*
 * screen.c - Graphics mode on/off and the back buffer.
 *
 * Uses: bga_* (gui/bga), kmalloc/kfree (memory_management) for the
 *       3 MiB back buffer, vga_fullscreen / vga_restore_display (vga/):
 *       the console stays away while graphics runs, and comes back after.
 */

#include "screen.h"
#include "bga.h"
#include "kheap.h"
#include "vga.h"

static canvas_t back;           // what we draw on
static canvas_t front;          // the visible framebuffer
static int is_open;

int screen_open(void) {
    if (is_open)
        return 1;
    if (!bga_is_available())
        return 0;

    back.width  = front.width  = SCREEN_WIDTH;
    back.height = front.height = SCREEN_HEIGHT;
    back.pixels = kmalloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    if (!back.pixels)
        return 0;

    vga_fullscreen(1);          // the console must not draw meanwhile
    bga_set_video_mode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP);
    front.pixels = bga_framebuffer();
    is_open = 1;
    return 1;
}

void screen_close(void) {
    if (!is_open)
        return;
    bga_disable();              // graphics off...
    kfree(back.pixels);
    back.pixels = 0;
    is_open = 0;
    vga_restore_display();      // ...the console's display set up again
    vga_fullscreen(0);          // ...and its text drawn
}

canvas_t *screen_canvas(void) {
    return &back;
}

void screen_present(void) {
    canvas_blit(&front, &back, 0, 0);
}
