/*
 * def_mouse_bounds.c - DEFENCE mouse_bounds: the pointer stays on screen.
 *
 * Stops:  mouse_coordinate_escape (security/attack/mouse_coordinate_escape)
 * Called: the mouse packet decoder (mouse/), for every new x and y.
 * Rule:   0 <= x <= 1023 and 0 <= y <= 767
 * Why:    the pointer position is used as a pixel index when the GUI
 *         draws it and checks clicks. Moving left from x = 0 would give a
 *         negative index: writes before the screen buffer.
 *         Hitting the edge is normal, so clamping is NOT logged.
 */

#include "def_mouse_bounds.h"
#include "registry.h"
#include "mouse.h"

int def_mouse_clamp(int value, int max) {
    if (max < 0 || value < 0) return 0;
    if (value > max) return max;
    return value;
}

static int selftest(void) {
    if (def_mouse_clamp(500, MOUSE_SCREEN_MAX_X) != 500) return 0;
    if (def_mouse_clamp(-1, MOUSE_SCREEN_MAX_X) != 0) return 0;
    if (def_mouse_clamp(-100000, MOUSE_SCREEN_MAX_Y) != 0) return 0;
    if (def_mouse_clamp(MOUSE_SCREEN_MAX_X + 1, MOUSE_SCREEN_MAX_X) != MOUSE_SCREEN_MAX_X) return 0;
    if (def_mouse_clamp(100000, MOUSE_SCREEN_MAX_Y) != MOUSE_SCREEN_MAX_Y) return 0;
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
