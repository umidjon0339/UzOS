/*
 * cmd_startx.c - `startx`: a small graphical desktop. ESC (or Ctrl+C) goes back.
 *
 * Every frame (about 30 per second):
 *   1. read the mouse, let the window manager handle clicks and drags
 *   2. once a second, refresh the "System Monitor" window with real data
 *   3. draw everything into the back buffer and show it
 *   4. sleep, so the rest of the system keeps running
 *
 * Uses: screen_* / wm_* / canvas_* (gui/), mouse_get_state (mouse/),
 *       keyboard_pop_key (keyboard/), timer_uptime_ms (timer/),
 *       pmm_* (memory_management), process_count_free_slots, sys_sleep
 *       and signal_interrupted (process_management), ui_* (ui module).
 */

#include "device_commands.h"
#include "screen.h"
#include "window_manager.h"
#include "mouse.h"
#include "keyboard.h"
#include "timer.h"
#include "pmm.h"
#include "process.h"
#include "sys_sleep.h"
#include "signal.h"
#include "ui.h"

#define FRAME_MS   33           // ~30 frames per second
#define REFRESH_MS 1000

// Print 'value' into 'out' after 'label' ("RAM free: 118 MB").
static void format_line(char *out, const char *label, uint32_t value, const char *unit) {
    char digits[12];
    int n = 0;
    do { digits[n++] = '0' + value % 10; value /= 10; } while (value);

    while (*label) *out++ = *label++;
    while (n) *out++ = digits[--n];
    while (*unit) *out++ = *unit++;
    *out = '\0';
}

static void draw_monitor(canvas_t *c) {
    char line[48];
    uint32_t kb_per_frame = 4;       // each frame is 4 KiB

    canvas_fill(c, COLOR_WHITE);
    format_line(line, "Uptime     : ", timer_uptime_ms() / 1000, " s");
    canvas_draw_string(c, 10, 10, line, COLOR_BLACK, COLOR_WHITE);
    format_line(line, "RAM total  : ", pmm_get_max_blocks() * kb_per_frame / 1024, " MB");
    canvas_draw_string(c, 10, 30, line, COLOR_BLACK, COLOR_WHITE);
    format_line(line, "RAM free   : ", pmm_get_free_blocks() * kb_per_frame / 1024, " MB");
    canvas_draw_string(c, 10, 50, line, COLOR_BLACK, COLOR_WHITE);
    format_line(line, "Processes  : ", MAX_PROCESSES - process_count_free_slots(), "");
    canvas_draw_string(c, 10, 70, line, COLOR_BLACK, COLOR_WHITE);

    // A bar showing how much RAM is used.
    uint32_t used = pmm_get_used_blocks(), total = pmm_get_max_blocks();
    int bar = total ? (int)(380 * used / total) : 0;
    canvas_fill_rect(c, 10, 100, 380, 16, COLOR_LIGHT_GREY);
    canvas_fill_rect(c, 10, 100, bar, 16, COLOR_GREEN);
    canvas_draw_string(c, 10, 122, "RAM used", COLOR_BLACK, COLOR_WHITE);
}

static void draw_welcome(canvas_t *c) {
    canvas_draw_string(c, 20, 30, "Welcome to the UzOS desktop!", COLOR_BLUE, COLOR_WHITE);
    canvas_draw_string(c, 20, 60, "Drag a window by its title bar.\n"
                                  "Click a window to raise it.\n\n"
                                  "Press ESC to go back to the shell.",
                       COLOR_BLACK, COLOR_WHITE);
}

// Drain the keyboard queue; 1 if ESC was among the keys.
static int escape_pressed(void) {
    char c;
    while ((c = keyboard_pop_key()))
        if (c == KEY_ESCAPE)
            return 1;
    return 0;
}

void cmd_startx(const char *args) {
    (void)args;

    if (!screen_open()) {
        ui_error("startx", "no BGA video card (or no memory for the screen buffer)");
        return;
    }

    wm_init();
    int monitor = wm_create_window(100, 150, 400, 140, "System Monitor");
    int welcome = wm_create_window(420, 330, 320, 160, "Hello UzOS");
    if (welcome >= 0)
        draw_welcome(wm_window_canvas(welcome));

    uint32_t last_refresh = 0;
    int first = 1;
    while (!escape_pressed() && !signal_interrupted()) {    // ESC or Ctrl+C
        mouse_state_t mouse;
        mouse_get_state(&mouse);
        wm_handle_mouse(&mouse);

        if (monitor >= 0 && (first || timer_uptime_ms() - last_refresh >= REFRESH_MS)) {
            draw_monitor(wm_window_canvas(monitor));
            last_refresh = timer_uptime_ms();
            first = 0;
        }

        wm_draw(screen_canvas(), &mouse);
        screen_present();
        sys_sleep(FRAME_MS);
    }

    wm_destroy_all();
    screen_close();
    ui_ok("startx", "back from the desktop");
}
