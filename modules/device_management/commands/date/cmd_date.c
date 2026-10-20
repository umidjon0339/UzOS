/*
 * cmd_date.c - `date`: today's date and time from the real-time clock.
 *
 * Uses: rtc_read / rtc_unix_time (rtc/), SYS_TIME (syscalls/time),
 *       ui_* (ui module).
 */

#include "device_commands.h"
#include "rtc.h"
#include "sys_time.h"
#include "ui.h"

void cmd_date(const char *args) {
    (void)args;
    rtc_time_t now;

    if (!rtc_read(&now)) {
        ui_error("date", "the clock gave an impossible date (see `seclab log`)");
        return;
    }

    ui_color(UI_TITLE);
    vga_print_str(GLYPH_CLOCK " ");
    ui_reset();
    ui_printf("%s %d %s %d  %02d:%02d:%02d\n",
              rtc_weekday_name(now.weekday), now.day, rtc_month_name(now.month),
              now.year, now.hour, now.minute, now.second);
    ui_color(UI_DIM);
    ui_printf("  unix time %u: seconds since 1970 (syscall %d gives it to programs)\n",
              rtc_unix_time(&now), SYS_TIME);
    ui_reset();
}
