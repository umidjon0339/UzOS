/*
 * cmd_nice.c - `nice [pid [level]]`: see and change priorities.
 *
 *   nice              every process: base level, effective level (with
 *                     aging), how long it has been waiting
 *   nice 5            the priority of PID 5
 *   nice 5 2          set PID 5 to level 2 (1 = low ... 7 = high)
 *
 * The shell runs as root, so it may raise or lower any normal process;
 * system processes (idle, kernel_main, statusd) keep their level. The
 * check is the defence priority_bounds, inside priority_set().
 *
 * Uses: priority_set / priority_effective (core/priority), the process
 *       table and the current process (process_management), arg_next /
 *       atoi (lib), box_* / ui_* (ui module).
 */

#include "cpu_commands.h"
#include "priority.h"
#include "scheduler.h"
#include "args.h"
#include "stdlib.h"
#include "format.h"
#include "ui.h"
#include "box.h"

#define WIDTH      58
#define NAME_WIDTH 18

static const char *state_name(process_state_t state) {
    switch (state) {
        case PROCESS_STATE_READY:    return "READY";
        case PROCESS_STATE_RUNNING:  return "RUNNING";
        case PROCESS_STATE_SLEEPING: return "SLEEPING";
        case PROCESS_STATE_ZOMBIE:   return "ZOMBIE";
        default:                     return "?";
    }
}

static void list_priorities(void) {
    process_t *table = process_get_table();

    box_begin("Priorities (7 runs first, 0 = idle only)", WIDTH);
    box_row();
    ui_color(UI_TITLE);
    vga_print_str("  PID  PRI  EFF  waited  STATE     NAME");
    box_row_end();

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state == PROCESS_STATE_FREE)
            continue;
        int effective = priority_effective(p);

        box_row();
        ui_printf("%5u  %3d  ", p->pid, p->priority);
        ui_color(effective > p->priority ? UI_ACCENT : ui_text_fg());   // aging at work
        ui_printf("%3d", effective);
        ui_reset();
        ui_printf("  %6u  %-8s  ", p->waited_ticks, state_name(p->state));
        ui_cell(p->name, NAME_WIDTH);
        box_row_end();
    }
    box_end();
    ui_hint("EFF = PRI + aging (1 level per 100 ms waited).  nice <pid> <level>");
}

static void show_one(process_t *p) {
    ui_start(UI_INFO, "nice");
    ui_printf("PID %u: priority %d, effective %d (waited %u ticks)",
              p->pid, p->priority, priority_effective(p), p->waited_ticks);
    ui_end();
}

void cmd_nice(const char *args) {
    char word[16];
    const char *rest = arg_next(args, word, sizeof(word));

    if (!word[0]) {
        list_priorities();
        return;
    }

    process_t *p = process_get_by_pid(atoi(word));
    if (!p) {
        ui_error("nice", "no such process");
        return;
    }

    arg_next(rest, word, sizeof(word));
    if (!word[0]) {
        show_one(p);
        return;
    }

    int level = atoi(word);
    if (!priority_set(scheduler_get_current_process(), p->pid, level)) {
        ui_error("nice", "refused: levels 1-7 only, and system processes keep theirs");
        return;
    }
    char text[48];
    format(text, sizeof(text), "PID %u now has priority %d", p->pid, level);
    ui_ok("nice", text);
}
