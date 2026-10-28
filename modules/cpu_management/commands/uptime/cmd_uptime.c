/*
 * cmd_uptime.c - `uptime [-p|-s]`: time, uptime, users, load average.
 *
 *    19:19:03 up 5 min,  1 user,  load average: 0.00, 0.01, 0.05
 *
 *   uptime -p     "up 5 minutes" (pretty)
 *   uptime -s     when the system started: "2026-10-26 19:14:00"
 *
 * The three load averages (1, 5, 15 minutes) say how many processes
 * wanted the CPU on average: see core/loadavg. "1 user": UzOS has one
 * console, so one logged-in session.
 *
 * Uses: loadavg_get / loadavg_format (core/loadavg), timer_uptime_ms
 *       (device_management/timer), rtc_read / rtc_unix_time
 *       (device_management/rtc), ui_* (ui), strcmp (lib/string).
 */

#include "modules/cpu_management/commands/uptime/cmd_uptime.h"  // this command
#include "modules/cpu_management/core/loadavg/loadavg.h"        // loadavg_get, loadavg_format
#include "modules/device_management/timer/timer.h"              // timer_uptime_ms
#include "modules/device_management/rtc/rtc.h"                  // rtc_read
#include "modules/ui/core/style/ui.h"                           // ui_printf, ui_usage
#include "lib/string/string.h"                                  // strcmp

#define SECONDS_PER_DAY (24 * 3600)

// "5 min", "1:05", "2 days, 1:05": how Linux's uptime writes it.
static void print_up(uint32_t s) {
    uint32_t days = s / SECONDS_PER_DAY, hours = s / 3600 % 24, minutes = s / 60 % 60;
    if (days)
        ui_printf("%u day%s, ", days, days == 1 ? "" : "s");
    if (hours)
        ui_printf("%2u:%02u", hours, minutes);
    else
        ui_printf("%u min", minutes);
}

static void print_pretty(uint32_t s) {
    uint32_t days = s / SECONDS_PER_DAY, hours = s / 3600 % 24, minutes = s / 60 % 60;
    vga_print_str("up ");
    if (days)
        ui_printf("%u day%s, ", days, days == 1 ? "" : "s");
    if (hours)
        ui_printf("%u hour%s, ", hours, hours == 1 ? "" : "s");
    ui_printf("%u minute%s\n", minutes, minutes == 1 ? "" : "s");
}

void cmd_uptime(const char *args) {
    uint32_t seconds = timer_uptime_ms() / 1000;
    rtc_time_t now;
    int have_clock = rtc_read(&now);

    if (strcmp(args, "-p") == 0) {
        print_pretty(seconds);
        return;
    }
    if (strcmp(args, "-s") == 0) {
        if (!have_clock) {
            ui_error("uptime", "the real-time clock gives no date");
            return;
        }
        // Count back from now: the time of day, minus how long we are up.
        int32_t day_s = now.hour * 3600 + now.minute * 60 + now.second - (int32_t)seconds;
        int days_back = 0;
        while (day_s < 0) {
            day_s += SECONDS_PER_DAY;
            days_back++;
        }
        ui_printf("%d-%02d-%02d %02d:%02d:%02d%s\n", now.year, now.month, now.day,
                  day_s / 3600, day_s / 60 % 60, day_s % 60,
                  days_back ? "  (days earlier than shown)" : "");
        return;
    }
    if (args[0]) {
        ui_usage("uptime [-p|-s]");
        return;
    }

    uint32_t load[3];
    char text[3][12];
    loadavg_get(load);
    for (int i = 0; i < 3; i++)
        loadavg_format(load[i], text[i], sizeof(text[i]));

    if (have_clock)
        ui_printf(" %02d:%02d:%02d ", now.hour, now.minute, now.second);
    vga_print_str("up ");
    print_up(seconds);
    ui_printf(",  1 user,  load average: %s, %s, %s\n", text[0], text[1], text[2]);
}
