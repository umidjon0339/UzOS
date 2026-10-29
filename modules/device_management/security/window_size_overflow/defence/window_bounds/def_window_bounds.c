/*
 * def_window_bounds.c - DEFENCE window_bounds: a window must fit the screen.
 *
 * Stops:  window_size_overflow (security/window_size_overflow)
 * Called: wm_create_window() (gui/wm/window_manager.c), before it
 *         allocates the window's canvas.
 * Rule:   0 < width <= WINDOW_MAX_WIDTH, 0 < height <= WINDOW_MAX_HEIGHT.
 * Why:    wm_create_window() does kmalloc(width * height * 4). A giant
 *         size overflows that multiplication (so kmalloc returns a tiny
 *         buffer that later draws overrun) or simply drains the heap.
 */

#include "def_window_bounds.h"
#include "registry.h"
#include "audit.h"

static int size_ok(int width, int height) {
    return width > 0 && height > 0 &&
           width <= WINDOW_MAX_WIDTH && height <= WINDOW_MAX_HEIGHT;
}

int def_window_size_ok(int width, int height) {
    if (size_ok(width, height))
        return 1;
    audit_log("window_bounds", "window refused: size is zero or bigger than the screen");
    return 0;
}

static int selftest(void) {
    if (!size_ok(100, 100)) return 0;                    // a normal window
    if (!size_ok(WINDOW_MAX_WIDTH, WINDOW_MAX_HEIGHT)) return 0;
    if (size_ok(0, 100)) return 0;                       // zero width
    if (size_ok(100, 0)) return 0;                       // zero height
    if (size_ok(-5, 100)) return 0;                      // negative
    if (size_ok(100000, 100000)) return 0;               // kmalloc overflow
    if (size_ok(WINDOW_MAX_WIDTH + 1, 100)) return 0;    // wider than screen
    return 1;
}

static const defence_t defence = {
    "window_bounds",
    "a window must be positive and no bigger than the screen",
    "window_size_overflow",
    selftest
};

void def_window_bounds_register(void) { registry_add_defence(&defence); }
