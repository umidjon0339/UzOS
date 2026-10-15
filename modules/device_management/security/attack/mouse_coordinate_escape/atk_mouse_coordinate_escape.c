#include "device_security.h"
#include "proc_security.h"
#include "vga.h"

static int atk_mouse_coordinate_escape_check(void) {
    int findings = 0;

    if (device_clamp_coordinate(-20, MOUSE_SCREEN_MAX_X) == 0) findings++;
    if (device_clamp_coordinate(1200, MOUSE_SCREEN_MAX_X) ==
        MOUSE_SCREEN_MAX_X) findings++;
    if (device_clamp_coordinate(-1, MOUSE_SCREEN_MAX_Y) == 0) findings++;
    if (device_clamp_coordinate(900, MOUSE_SCREEN_MAX_Y) ==
        MOUSE_SCREEN_MAX_Y) findings++;
    return findings;
}

static int atk_mouse_coordinate_escape_simulate(void) {
    vga_print_str("  probes decoded coordinates beyond all four display edges\n");
    vga_print_str("  live mouse state is unchanged\n");
    return atk_mouse_coordinate_escape_check();
}

static int atk_mouse_coordinate_escape_launch(void) {
    vga_print_str("  checking the same coordinate bounds used by the mouse IRQ\n");
    return atk_mouse_coordinate_escape_check();
}

static const attack_t atk = {
    "mouse_coordinate_escape",
    "large or negative mouse deltas can move the pointer outside the display",
    INV_CONSISTENCY,
    SEV_MEDIUM,
    atk_mouse_coordinate_escape_simulate,
    atk_mouse_coordinate_escape_launch,
    0,
    "mouse_bounds"
};

void atk_mouse_coordinate_escape_register(void) {
    proc_security_register_attack(&atk);
}
