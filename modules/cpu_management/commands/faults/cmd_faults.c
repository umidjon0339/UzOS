/*
 * cmd_faults.c - `faults [list]`: CPU exceptions and the processes they ended.
 *
 *   faults        how many processes each exception killed, and the
 *                 newest faults: who, where (EIP), which instruction
 *   faults list   all 20 exceptions the CPU defines, with what they mean
 *
 * Every number here is a crash the OS survived: a fault in the kernel
 * itself halts the machine and never gets counted.
 *
 * Uses: cpu_fault_* (core/faults), isr_exception_name (arch/x86/isr),
 *       timer_frequency (device_management), box_* / ui_* (ui module).
 */

#include "cpu_commands.h"
#include "cpu_faults.h"
#include "timer.h"
#include "string.h"
#include "ui.h"
#include "box.h"

#define WIDTH          66
#define LIST_WIDTH     78       // the whole 80-column screen
#define NAME_WIDTH     10
#define DEFINED_VECTORS 20      // 0-19; 20-31 are rare or reserved

static void print_counts(void) {
    int any = 0;

    box_row();
    ui_color(UI_TITLE);
    vga_print_str("  #  exception                       killed");
    box_row_end();

    for (int v = 0; v < ISR_EXCEPTIONS; v++) {
        uint32_t n = cpu_fault_kills(v);
        if (!n)
            continue;
        any = 1;
        box_row();
        ui_printf(" %2d  ", v);
        ui_cell(isr_exception_name(v), 30);
        ui_color(UI_ACCENT);
        ui_printf("%7u", n);
        box_row_end();
    }

    if (!any) {
        box_row();
        ui_color(UI_DIM);
        vga_print_str(" none yet: try  seclab attack user_exception live");
        box_row_end();
    }
}

static void print_recent(void) {
    box_row();
    ui_color(UI_TITLE);
    vga_print_str("  PID  NAME       #   EIP         instruction  at");
    box_row_end();

    for (int i = 0; i < cpu_fault_log_count(); i++) {
        const cpu_fault_t *f = cpu_fault_log_get(i);
        uint32_t s = f->tick / (timer_frequency() ? timer_frequency() : 1);

        box_row();
        ui_printf("%5u  ", f->pid);
        ui_cell(f->name, NAME_WIDTH);
        ui_printf(" %2u  0x%08x  ", f->vector, f->eip);
        ui_cell(f->instruction ? f->instruction : "?", 11);
        ui_color(UI_DIM);
        ui_printf("  %u:%02u", s / 60, s % 60);
        box_row_end();
    }
}

static void faults_summary(void) {
    box_begin("CPU exceptions from ring 3 (each one killed a process)", WIDTH);
    print_counts();
    if (cpu_fault_log_count()) {
        box_divider();
        print_recent();
    }
    box_divider();
    box_row();
    ui_color(UI_DIM);
    ui_printf("total: %u killed, the kernel never stopped", cpu_fault_total());
    box_row_end();
    box_end();
    ui_hint("faults list: what every exception means");
}

static void faults_list(void) {
    box_begin("CPU exceptions (vectors 0-19)", LIST_WIDTH);
    for (int v = 0; v < DEFINED_VECTORS; v++) {
        box_row();
        ui_color(UI_ACCENT);
        ui_printf("%2d ", v);
        ui_reset();
        ui_cell(isr_exception_name(v), 28);
        ui_color(UI_DIM);
        ui_cell(cpu_fault_hint(v), LIST_WIDTH - 4 - 31);
        box_row_end();
    }
    box_end();
}

void cmd_faults(const char *args) {
    if (strcmp(args, "list") == 0)
        faults_list();
    else if (args[0])
        ui_usage("faults [list]");
    else
        faults_summary();
}
