/*
 * cmd_cpustat.c - `cpustat`: where the CPU's time went.
 *
 *   ┌─ CPU load (last second) ───────────────────┐
 *   │ Busy    ██░░░░░░░░░░░░░░░░░░    4%         │
 *   │ ring 3     0%  user programs               │
 *   │ ring 0     4%  the kernel and its tasks    │
 *   │ idle      96%  nothing to do: hlt          │
 *   ├────────────────────────────────────────────┤
 *   │  PID  CPU%   user ms  kernel ms  NAME      │
 *
 * The numbers come from sampling: every timer tick is charged to the
 * process it interrupted (core/accounting).
 *
 * Uses: cpu_load / cpu_account_ms / cpu_busy_ms / cpu_idle_ms
 *       (core/accounting), tsc_mhz (core/tsc), the process table and
 *       scheduler_idle_process (process_management), timer_frequency
 *       (device_management), box_* / ui_* (ui module).
 */

#include "cpu_commands.h"
#include "cpu_account.h"
#include "tsc.h"
#include "scheduler.h"
#include "timer.h"
#include "ui.h"
#include "box.h"

#define WIDTH      62
#define NAME_WIDTH 18
#define BAR_WIDTH  30

static void share_row(const char *label, uint32_t value, const char *meaning) {
    box_row();
    ui_label(label, 8);
    ui_printf("%4u%%  ", value);
    ui_color(UI_DIM);
    vga_print_str(meaning);
    box_row_end();
}

static uint32_t ms_of(uint32_t ticks) {
    uint32_t hz = timer_frequency();
    return hz ? (uint32_t)((uint64_t)ticks * 1000 / hz) : 0;
}

static void print_processes(void) {
    process_t *table = process_get_table();

    box_row();
    ui_color(UI_TITLE);
    vga_print_str("  PID  CPU%   user ms  kernel ms  NAME");
    box_row_end();

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state == PROCESS_STATE_FREE)
            continue;
        box_row();
        ui_printf("%5u  ", p->pid);
        if (p == scheduler_idle_process())
            ui_color(UI_DIM);               // idle time is free time, not load
        else
            ui_color(p->cpu_percent >= 50 ? UI_WARNING : ui_text_fg());
        ui_printf("%3u%%", p->cpu_percent);
        ui_reset();
        ui_printf("  %8u  %9u  ", ms_of(p->user_ticks), ms_of(p->kernel_ticks));
        ui_cell(p->name, NAME_WIDTH);
        box_row_end();
    }
}

void cmd_cpustat(const char *args) {
    (void)args;
    const cpu_load_t *load = cpu_load();

    box_begin("CPU load (last second)", WIDTH);
    box_row();
    ui_label("Busy", 8);
    ui_bar(load->busy, 100, BAR_WIDTH);
    ui_printf(" %3u%%", load->busy);
    box_row_end();
    share_row("ring 3", load->user, "user programs");
    share_row("ring 0", load->kernel, "the kernel and its tasks");
    share_row("idle", load->idle, "nothing to do: the idle task halts");

    box_row();
    ui_color(UI_DIM);
    ui_printf("since boot: busy %u.%u s, idle %u.%u s, clock %u MHz",
              cpu_busy_ms() / 1000, cpu_busy_ms() / 100 % 10,
              cpu_idle_ms() / 1000, cpu_idle_ms() / 100 % 10, tsc_mhz());
    box_row_end();

    box_divider();
    print_processes();
    box_end();
}
