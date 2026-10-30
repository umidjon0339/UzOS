/*
 * def_mouse_bounds.c - DEFENCE mouse_bounds: the pointer stays on screen.
 *
 * Stops:  mouse_coordinate_escape (security/mouse_coordinate_escape)
 * Called: the mouse packet decoder (mouse/), for every new x and y, and
 *         mouse_set_screen() when the screen size changes.
 * Rule:   0 <= x <= width - 1 and 0 <= y <= height - 1 of the current
 *         screen (1023 and 767 at 1024 x 768)
 * Why:    the pointer position is used as a pixel index when the GUI
 *         draws it and checks clicks. Moving left from x = 0 would give a
 *         negative index: writes before the screen buffer.
 *         Hitting the edge is normal, so clamping is NOT logged.
 */

#include "def_mouse_bounds.h"
#include "registry.h"

int def_mouse_clamp(int value, int max) {
    if (max < 0 || value < 0) return 0;
    if (value > max) return max;
    return value;
}

static int selftest(void) {
    // A 1024 x 768 screen: the last pixel is (1023, 767).
    if (def_mouse_clamp(500, 1023) != 500) return 0;
    if (def_mouse_clamp(-1, 1023) != 0) return 0;
    if (def_mouse_clamp(-100000, 767) != 0) return 0;
    if (def_mouse_clamp(1024, 1023) != 1023) return 0;
    if (def_mouse_clamp(100000, 767) != 767) return 0;
    // Shrunk to 800 x 600: a pointer at x = 1000 is pulled back to 799.
    if (def_mouse_clamp(1000, 799) != 799) return 0;
    return 1;
}

static const defence_t defence = {
    "mouse_bounds",
    "clamp the mouse pointer to the screen after every move",
    "mouse_coordinate_escape",
    selftest
};

void def_mouse_bounds_register(void) {
    registry_add_defence(&defence);
}
