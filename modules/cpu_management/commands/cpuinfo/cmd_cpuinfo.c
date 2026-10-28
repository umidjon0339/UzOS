/*
 * cmd_cpuinfo.c - `cpuinfo [flags]`: what the CPU is and how it is set up.
 *
 *   cpuinfo         vendor, name, signature, features, CR0/CR4 switches
 *   cpuinfo flags   every feature UzOS knows, with what it means
 *
 * Features come from cpuid (what the chip CAN do). Control registers
 * show what the kernel has actually TURNED ON.
 *
 * Uses: cpu_info / cpu_has (core/info), tsc_mhz (core/tsc), cpu_read_cr0 / cpu_read_cr4
 *       (arch/x86/cpu), box_* / ui_* (ui module).
 */

#include "modules/cpu_management/commands/cpuinfo/cmd_cpuinfo.h"  // this command
#include "modules/cpu_management/core/info/cpu_info.h"            // cpu_info, cpu_has
#include "modules/cpu_management/core/tsc/tsc.h"                  // tsc_mhz
#include "arch/x86/cpu/cpu.h"                                     // cpu_read_cr0/cr4, CR*_*
#include "lib/string/string.h"                                    // strcmp
#include "lib/format/format.h"                                    // format
#include "modules/ui/core/style/ui.h"                             // ui_*
#include "modules/ui/core/box/box.h"                              // box_*

#define WIDTH        62
#define LABEL_WIDTH  11
#define FLAGS_PER_ROW 6

// The control register switches worth explaining.
static const struct {
    const char *reg;
    uint32_t bit;
    const char *name;
    const char *meaning;
} switches[] = {
    { "CR0", CR0_PE,  "PE",  "protected mode: rings and segments" },
    { "CR0", CR0_PG,  "PG",  "paging: virtual addresses" },
    { "CR0", CR0_WP,  "WP",  "ring 0 obeys read-only pages" },
    { "CR0", CR0_EM,  "EM",  "FPU instructions fault (#7)" },
    { "CR4", CR4_TSD, "TSD", "rdtsc only in ring 0" },
    { "CR4", CR4_PSE, "PSE", "4 MiB pages allowed" },
    { "CR4", CR4_PAE, "PAE", "64-bit page table entries" },
    { "CR4", CR4_PGE, "PGE", "global pages" },
};
#define SWITCH_COUNT (sizeof(switches) / sizeof(switches[0]))

static void text_row(const char *label, const char *text) {
    box_row();
    ui_label(label, LABEL_WIDTH);
    ui_cell(text, WIDTH - 4 - LABEL_WIDTH);
    box_row_end();
}

static void print_features(void) {
    for (int f = 0; f < CPU_FEAT_COUNT; f++) {
        if (f % FLAGS_PER_ROW == 0) {
            box_row();
            ui_label(f == 0 ? "Features" : "", LABEL_WIDTH);
        }
        int has = cpu_has(f);
        ui_color(has ? UI_GOOD : UI_DIM);
        vga_print_str(has ? GLYPH_CHECK : "-");
        ui_printf(" %-6s", cpu_feature_name(f));     // 1 + 1 + 6 = 8 columns
        ui_reset();
        if (f % FLAGS_PER_ROW == FLAGS_PER_ROW - 1 || f == CPU_FEAT_COUNT - 1)
            box_row_end();
    }
}

static void print_switches(void) {
    uint32_t cr0 = cpu_read_cr0(), cr4 = cpu_read_cr4();

    for (unsigned i = 0; i < SWITCH_COUNT; i++) {
        uint32_t value = switches[i].reg[2] == '0' ? cr0 : cr4;
        int on = (value & switches[i].bit) != 0;
        int first = i == 0 || strcmp(switches[i].reg, switches[i - 1].reg) != 0;

        box_row();
        ui_label(first ? switches[i].reg : "", 5);
        ui_printf("%-4s", switches[i].name);
        ui_color(on ? UI_GOOD : UI_DIM);
        ui_printf("%-4s", on ? "on" : "off");
        ui_reset();
        ui_cell(switches[i].meaning, WIDTH - 4 - 13);
        box_row_end();
    }
}

static void cpuinfo_summary(void) {
    const cpu_info_t *cpu = cpu_info();
    char text[64];

    box_begin("CPU", WIDTH);
    text_row("Vendor", cpu->vendor);
    text_row("Name", cpu->brand[0] ? cpu->brand : "(the CPU has no brand string)");
    format(text, sizeof(text), "family %u, model %u, stepping %u",
           cpu->family, cpu->model, cpu->stepping);
    text_row("Signature", text);
    if (tsc_mhz())
        format(text, sizeof(text), "%u MHz (TSC cycles counted over %u PIT ticks)",
               tsc_mhz(), TSC_CALIBRATE_TICKS);
    else
        format(text, sizeof(text), "unknown (no TSC to measure with)");
    text_row("Clock", text);

    box_divider();
    print_features();

    box_divider();
    box_row();
    ui_color(UI_TITLE);
    vga_print_str("Control registers: what the kernel switched on");
    box_row_end();
    print_switches();
    box_end();
    ui_hint("cpuinfo flags: what every feature means");
}

static void cpuinfo_flags(void) {
    box_begin("CPU features (cpuid leaf 1)", WIDTH);
    for (int f = 0; f < CPU_FEAT_COUNT; f++) {
        int has = cpu_has(f);
        box_row();
        ui_color(has ? UI_GOOD : UI_DIM);
        vga_print_str(has ? GLYPH_CHECK : "-");
        ui_printf(" %-6s", cpu_feature_name(f));
        ui_reset();
        ui_cell(cpu_feature_meaning(f), 32);
        ui_color(has ? UI_GOOD : UI_DIM);
        vga_print_str(has ? " yes" : " no");
        box_row_end();
    }
    box_end();
}

void cmd_cpuinfo(const char *args) {
    if (strcmp(args, "flags") == 0)
        cpuinfo_flags();
    else if (args[0])
        ui_usage("cpuinfo [flags]");
    else
        cpuinfo_summary();
}
