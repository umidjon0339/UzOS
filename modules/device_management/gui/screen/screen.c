/*
 * screen.c - Graphics mode on/off, the screen size, and the back buffer.
 *
 * The back buffer is width * height * 4 bytes: 1920 x 1080 needs 8.3 MB,
 * more than the whole kernel heap (8 MiB). So it does not come from
 * kmalloc. It has its own addresses, BACKBUFFER_START..END, and RAM frames
 * are mapped there only while the desktop runs:
 *
 *   open / resize:  for each 4 KiB page: take a free frame, map it
 *   close / resize: for each 4 KiB page: unmap it, give the frame back
 *
 * Uses: bga_* (gui/bga), pmm_alloc_block / pmm_free_block and
 *       vmm_map_page / vmm_unmap_page / vmm_walk (memory_management),
 *       vga_fullscreen / vga_restore_display (vga/): the console stays
 *       away while graphics runs, and comes back after.
 */

#include "screen.h"
#include "bga.h"
#include "pmm.h"
#include "vmm.h"
#include "vga.h"

static canvas_t back;           // what we draw on
static canvas_t front;          // the visible framebuffer
static uint32_t back_bytes;     // how much of the back buffer is mapped
static int is_open;

/* ---------- The back buffer's RAM ---------- */

static void unmap_back_buffer(void) {
    for (uint32_t offset = 0; offset < back_bytes; offset += PAGE_SIZE) {
        vmm_walk_t walk;
        if (vmm_walk(BACKBUFFER_START + offset, &walk)) {
            vmm_unmap_page(BACKBUFFER_START + offset);
            pmm_free_block((void *)walk.physical);
        }
    }
    back_bytes = 0;
}

static int map_back_buffer(uint32_t bytes) {
    if (bytes > BACKBUFFER_END - BACKBUFFER_START)
        return 0;                               // does not fit its addresses
    for (back_bytes = 0; back_bytes < bytes; back_bytes += PAGE_SIZE) {
        void *frame = pmm_alloc_block();
        if (!frame)
            break;
        if (!vmm_map_page(BACKBUFFER_START + back_bytes, (uint32_t)frame, PAGE_WRITABLE)) {
            pmm_free_block(frame);
            break;
        }
    }
    if (back_bytes < bytes) {                   // out of RAM: undo what we got
        unmap_back_buffer();
        return 0;
    }
    return 1;
}

/* ---------- Size ---------- */

// Switch the card to width x height and give the back buffer that size.
static int set_size(int width, int height) {
    if (!bga_set_video_mode(width, height, SCREEN_BPP))
        return 0;                               // refused: a bad size
    if (!map_back_buffer((uint32_t)width * height * sizeof(uint32_t)))
        return 0;

    back.width  = front.width  = width;
    back.height = front.height = height;
    back.pixels  = (uint32_t *)BACKBUFFER_START;
    front.pixels = bga_framebuffer();
    return 1;
}

int screen_open(int width, int height) {
    if (is_open)
        return 1;
    if (!bga_is_available())
        return 0;

    vga_fullscreen(1);          // the console must not draw meanwhile
    if (!set_size(width, height)) {
        vga_restore_display();  // put the console's own mode back
        vga_fullscreen(0);
        return 0;
    }
    is_open = 1;
    return 1;
}

int screen_resize(int width, int height) {
    if (!is_open)
        return 0;
    int old_width = back.width, old_height = back.height;

    unmap_back_buffer();
    if (set_size(width, height))
        return 1;
    set_size(old_width, old_height);    // could not: the frames we just gave back fit
    return 0;
}

int screen_width(void)  { return back.width; }
int screen_height(void) { return back.height; }

void screen_close(void) {
    if (!is_open)
        return;
    bga_disable();              // graphics off...
    unmap_back_buffer();
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
