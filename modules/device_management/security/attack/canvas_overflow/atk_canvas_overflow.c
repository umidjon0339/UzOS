/*
 * atk_canvas_overflow.c - ATTACK canvas_overflow: draw outside a buffer.
 *
 * Breaks:     isolation - a drawing stays inside its own pixels
 * Stopped by: canvas_clip (security/defence/canvas_clip)
 *
 * pixels[y * width + x] with a negative x, or a rectangle running past
 * the edge, writes into the memory next to the buffer.
 *
 * simulate(): show the index the old window code computed for x = -8.
 * launch():   make a small 16 x 16 canvas with "guard" words on both
 *             sides, draw shapes that hang over every edge, and check
 *             the guard words. Then free it.
 *
 * Uses: canvas_* (gui/canvas), kmalloc/kfree (memory_management).
 */

#include "registry.h"
#include "canvas.h"
#include "kheap.h"
#include "vga.h"

#define SIZE   16
#define GUARD  64               // words of guard before and after the pixels
#define MARK   0xDEADBEEF

static int simulate(void) {
    vga_print_str("  old window text code checked only the right/bottom edge:\n");
    vga_print_str("  x = -8, y = 0 -> index -8: 8 pixels BEFORE the buffer\n");
    return 1;
}

static int count_damaged(const uint32_t *memory) {
    int damaged = 0;
    for (int i = 0; i < GUARD; i++) {
        damaged += memory[i] != MARK;                           // before
        damaged += memory[GUARD + SIZE * SIZE + i] != MARK;     // after
    }
    return damaged;
}

static int launch(void) {
    uint32_t *memory = kmalloc((GUARD + SIZE * SIZE + GUARD) * sizeof(uint32_t));
    if (!memory)
        return 0;
    for (int i = 0; i < GUARD + SIZE * SIZE + GUARD; i++)
        memory[i] = MARK;

    canvas_t c = { memory + GUARD, SIZE, SIZE };

    canvas_fill_rect(&c, -8, -8, 12, 12, COLOR_RED);              // top-left corner
    canvas_fill_rect(&c, 10, 10, 1000, 1000, COLOR_RED);          // bottom-right
    canvas_fill_rect(&c, 0x7FFFFFF0, 0, 0x7FFFFFF0, 1, COLOR_RED); // x + w overflows
    canvas_draw_string(&c, -4, -3, "overflow", COLOR_WHITE, COLOR_BLACK);
    canvas_blit(&c, &c, -10, 12);                                 // a copy hanging out

    int damaged = count_damaged(memory);
    vga_print_str("  5 drawings over every edge; guard words damaged: ");
    vga_print_int(damaged);
    vga_print_str(damaged ? "  (OVERFLOW)\n" : "  (all clipped)\n");

    kfree(memory);
    return damaged;
}

static const attack_t attack = {
    "canvas_overflow",
    "drawing with negative or huge coordinates writes outside the pixel buffer",
    INV_ISOLATION, SEV_HIGH,
    simulate, launch, 0,        // launch frees its own canvas
    "canvas_clip"
};

void atk_canvas_overflow_register(void) {
    registry_add_attack(&attack);
}
