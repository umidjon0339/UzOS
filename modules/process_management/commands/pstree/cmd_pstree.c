/*
 * cmd_pstree.c - `pstree [-p] [pid]`: the processes as a family tree.
 *
 *   pstree -p
 *   idle(1)
 *    |-kernel_main(2)
 *    |  |-worker(5)
 *    |  `-sleep(6)
 *    `-flood_target(9)       an orphan: its parent ended, PID 1 took it
 *
 * The tree follows process_t.ppid. A process whose parent is gone is
 * re-parented when the parent ends (process_ended: to a subreaper, or
 * PID 1), so every branch hangs from a living process. Without -p the
 * PIDs are left out; with a pid, only that branch is drawn.
 *
 * Uses: process_get_table / process_get_by_pid (core/process), atoi, ui.
 */

#include "modules/process_management/commands/pstree/cmd_pstree.h"    // this file
#include "modules/process_management/core/process/process.h"          // process_get_table, process_get_by_pid
#include "lib/args/args.h"                                            // arg_next
#include "lib/stdlib/stdlib.h"                                        // atoi
#include "lib/string/string.h"                                        // strcmp
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_color

#define MAX_DEPTH 16

static int show_pids;
static char last_at[MAX_DEPTH];     // 1 = the branch at this depth is its parent's last

static int alive(const process_t *p) {
    return p->state != PROCESS_STATE_FREE;
}

static int is_child(const process_t *c, const process_t *parent) {
    return alive(c) && c != parent && c->ppid == parent->pid;
}

static void draw(const process_t *p, int depth) {
    process_t *table = process_get_table();

    for (int d = 1; d < depth; d++)
        ui_printf(last_at[d] ? "   " : " | ");
    if (depth > 0)
        ui_printf(last_at[depth] ? " `-" : " |-");
    ui_color(p->state == PROCESS_STATE_ZOMBIE ? UI_DIM : p->is_system ? UI_ACCENT : VGA_COLOR_WHITE);
    ui_printf("%s", p->name);
    ui_reset();
    if (show_pids)
        ui_printf("(%u)", p->pid);
    ui_printf("%s\n", p->state == PROCESS_STATE_STOPPED ? " [stopped]" : "");
    if (depth + 1 >= MAX_DEPTH)
        return;

    int last = -1;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (is_child(&table[i], p))
            last = i;
    for (int i = 0; i <= last; i++) {
        if (!is_child(&table[i], p))
            continue;
        last_at[depth + 1] = i == last;
        draw(&table[i], depth + 1);
    }
}

void cmd_pstree(const char *args) {
    char word[16];
    uint32_t root = 0;
    show_pids = 0;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-p") == 0)
            show_pids = 1;
        else if ((root = (uint32_t)atoi(word)) == 0) {
            ui_usage("pstree [-p] [pid]");
            return;
        }
    }

    if (root) {
        process_t *p = process_get_by_pid(root);
        if (!p) {
            ui_error("pstree", "no such process");
            return;
        }
        draw(p, 0);
        return;
    }

    // The roots: processes whose parent does not exist (idle, kernel_main).
    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (alive(&table[i]) && !process_get_by_pid(table[i].ppid))
            draw(&table[i], 0);
}
