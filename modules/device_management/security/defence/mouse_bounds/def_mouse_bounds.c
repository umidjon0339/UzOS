#include "device_security.h"
#include "proc_security.h"

int device_clamp_coordinate(int value, int maximum) {
    if (maximum < 0) return 0;
    if (value < 0) return 0;
    if (value > maximum) return maximum;
    return value;
}

static int def_mouse_bounds_selftest(void) {
    if (device_clamp_coordinate(0, MOUSE_SCREEN_MAX_X) != 0) return 0;
    if (device_clamp_coordinate(500, MOUSE_SCREEN_MAX_X) != 500) return 0;
    if (device_clamp_coordinate(-1, MOUSE_SCREEN_MAX_X) != 0) return 0;
    if (device_clamp_coordinate(1024, MOUSE_SCREEN_MAX_X) != MOUSE_SCREEN_MAX_X) return 0;
    if (device_clamp_coordinate(-1, MOUSE_SCREEN_MAX_Y) != 0) return 0;
    if (device_clamp_coordinate(768, MOUSE_SCREEN_MAX_Y) != MOUSE_SCREEN_MAX_Y) return 0;
    if (device_clamp_coordinate(1, -1) != 0) return 0;
    return 1;
}

static const defence_t def = {
    "mouse_bounds",
    "keep decoded mouse coordinates inside the display",
    "mouse_coordinate_escape",
    def_mouse_bounds_selftest
};

void def_mouse_bounds_register(void) {
    proc_security_register_defence(&def);
}
