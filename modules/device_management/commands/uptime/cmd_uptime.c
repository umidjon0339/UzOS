/*
 * cmd_uptime.c - `uptime`: how long the system has been running.
 *
 * Uses: timer_uptime_ms / timer_ticks / timer_frequency (timer/),
 *       ui_* (ui module).
 */

#include "device_commands.h"
#include "timer.h"
#include "ui.h"

void cmd_uptime(const char *args) {
    (void)args;
    uint32_t seconds = timer_uptime_ms() / 1000;

    ui_color(UI_TITLE);
    vga_print_str(GLYPH_CLOCK " ");
    ui_reset();
    ui_printf("up %uh %um %us", seconds / 3600, seconds / 60 % 60, seconds % 60);
    ui_color(UI_DIM);
    ui_printf("   (%u timer ticks at %u per second)", timer_ticks, timer_frequency());
    ui_end();
}
