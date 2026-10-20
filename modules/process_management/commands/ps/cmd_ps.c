/*
 * cmd_ps.c - `ps`: list every process, in a frame.
 *
 * Walks the process table and prints one row per used slot. Names go
 * through ui_cell(), so a name can never break the table (safe_text).
 * PRI is the priority level and CPU% the share of the last second, both
 * kept by cpu_management (`nice` and `cpustat` show more).
 *
 * Uses: process_get_table (core/process), users_name (user_management),
 *       box_* / ui_cell (ui module).
 */

#include "proc_commands.h"
#include "process.h"
#include "ui.h"
#include "box.h"
#include "users.h"

#define WIDTH       64
#define NAME_WIDTH  20

static const char *state_name(process_state_t state) {
    switch (state) {
        case PROCESS_STATE_READY:    return "READY";
        case PROCESS_STATE_RUNNING:  return "RUNNING";
        case PROCESS_STATE_SLEEPING: return "SLEEPING";
        case PROCESS_STATE_ZOMBIE:   return "ZOMBIE";
        default:                     return "?";
    }
}

static enum vga_color state_color(process_state_t state) {
    switch (state) {
        case PROCESS_STATE_RUNNING:  return UI_GOOD;
        case PROCESS_STATE_SLEEPING: return UI_LINK;
        case PROCESS_STATE_ZOMBIE:   return UI_BAD;
        default:                     return ui_text_fg();
    }
}

void cmd_ps(const char *args) {
    (void)args;
    process_t *table = process_get_table();

    box_begin("Processes", WIDTH);
    box_row();
    ui_color(UI_TITLE);
    ui_printf("  PID  %-9s PRI  CPU%%  USER   NAME", "STATE");
    box_row_end();

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state == PROCESS_STATE_FREE)
            continue;

        box_row();
        ui_printf("%5u  ", p->pid);
        ui_color(state_color(p->state));
        ui_cell(state_name(p->state), 9);
        ui_reset();
        char user[USER_NAME_MAX + 2];
        users_name(p->uid, user, sizeof(user));
        ui_printf("%3d  %3u%%  ", p->priority, p->cpu_percent);
        ui_cell(user, 6);
        vga_print_str(" ");
        ui_cell(p->name, NAME_WIDTH);
        ui_color(UI_DIM);
        vga_print_str(p->is_system ? " system" : "");
        box_row_end();
    }
    box_end();
}
