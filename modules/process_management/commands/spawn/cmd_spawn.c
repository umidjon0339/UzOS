/*
 * cmd_spawn.c - `spawn`: start a background worker process.
 *
 * The worker prints a heartbeat 3 times, 3 seconds apart, then exits.
 * Meanwhile the shell keeps working: that is multitasking.
 *
 * Uses: process_create (core/process), sys_sleep and sys_exit
 *       (syscalls/), sys_getpid to print its own PID, ui_* (ui module).
 */

#include "proc_commands.h"
#include "process.h"
#include "sys_sleep.h"
#include "sys_exit.h"
#include "sys_getpid.h"
#include "ui.h"

#define HEARTBEATS  3
#define HEARTBEAT_MS 3000

static void worker(void) {
    for (int i = 1; i <= HEARTBEATS; i++) {
        sys_sleep(HEARTBEAT_MS);    // sleeping: other processes run meanwhile

        uint8_t color = vga_get_color();    // the shell may be printing in any colour
        ui_color(UI_ACCENT);
        ui_printf("\n" GLYPH_GEAR " [worker %d] heartbeat %d/%d\n", sys_getpid(), i, HEARTBEATS);
        vga_set_color_byte(color);
    }
    sys_exit();
}

void cmd_spawn(const char *args) {
    (void)args;
    int pid = process_create("worker", worker, 0);

    if (pid < 0) {
        ui_error("spawn", "no free process slot (or refused, see `seclab log`)");
        return;
    }
    ui_start(UI_OK, "spawn");
    ui_printf("background worker started, PID %d", pid);
    ui_end();
}
