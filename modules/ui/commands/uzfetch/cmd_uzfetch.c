/*
 * cmd_uzfetch.c - `uzfetch`: the flag on the left, the system on the right.
 *
 * Like "neofetch" on Linux. Each info line is built into text first,
 * then the picture row and the info row are printed side by side:
 *
 *     [flag row 0]   guest@uzos
 *     [flag row 1]   ─────────
 *     [flag row 2]   OS      UzOS 1.0 i386
 *     ...
 *
 * Uses: logo_print_flag_row (core/logo), ui_bar (core/box), cpuid
 *       (arch/x86), and information from every module: pmm, the
 *       process table, the shell registry, the VFS, the security lab,
 *       the logged-in user, the display, palette, font, rtc and timer.
 */

#include "ui_commands.h"
#include "modules/network_management/core/hostname/hostname.h"   // hostname_get
#include "ui.h"
#include "box.h"
#include "logo.h"
#include "format.h"
#include "cpuid.h"
#include "pmm.h"
#include "process.h"
#include "shell.h"
#include "vfs.h"
#include "registry.h"
#include "palette.h"
#include "font.h"
#include "rtc.h"
#include "timer.h"
#include "string.h"
#include "session.h"

#define GAP          "   "
#define LABEL_WIDTH  10
#define MAX_LINES    16
#define TEXT_MAX     64

typedef struct {
    const char *label;          // 0 = the line is printed specially
    char text[TEXT_MAX];
} info_t;

static info_t lines[MAX_LINES];
static int line_count;

static char *add(const char *label) {
    info_t *line = &lines[line_count++];
    line->label = label;
    line->text[0] = '\0';
    return line->text;
}

/* ---------- Gathering ---------- */

static int count_nodes(vfs_node_t *node) {
    int n = 1;
    for (int i = 0; i < node->num_children; i++)
        n += count_nodes(node->children[i]);
    return n;
}

static int count_processes(void) {
    process_t *table = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        n += table[i].state != PROCESS_STATE_FREE;
    return n;
}

static int count_groups(void) {
    int groups = 0;
    for (int i = 0; i < shell_command_count(); i++) {
        int seen = 0;
        for (int j = 0; j < i; j++)
            seen |= strcmp(shell_command(j)->group, shell_command(i)->group) == 0;
        groups += !seen;
    }
    return groups;
}

static void gather(void) {
    char cpu[49];
    rtc_time_t now;
    uint32_t kb_per_frame = PAGE_SIZE / 1024;
    uint32_t s = timer_uptime_ms() / 1000;

    line_count = 0;
    add(0);                                                 // guest@uzos
    add(0);                                                 // underline
    format(add("OS"), TEXT_MAX, "UzOS 1.0 (i386, 32-bit)");
    format(add("Kernel"), TEXT_MAX, "multiboot, protected mode, paging");
    if (!cpuid_brand(cpu))
        cpuid_vendor(cpu);
    format(add("CPU"), TEXT_MAX, "%s", cpu);
    format(add("Memory"), TEXT_MAX, "%u / %u MB ",
           pmm_get_used_blocks() * kb_per_frame / 1024,
           pmm_get_max_blocks() * kb_per_frame / 1024);
    format(add("Uptime"), TEXT_MAX, "%uh %um %us", s / 3600, s / 60 % 60, s % 60);
    if (rtc_read(&now))
        format(add("Date"), TEXT_MAX, "%s %d %s %d, %02d:%02d",
               rtc_weekday_name(now.weekday), now.day, rtc_month_name(now.month),
               now.year, now.hour, now.minute);
    format(add("Processes"), TEXT_MAX, "%d running", count_processes());
    format(add("Commands"), TEXT_MAX, "%d in %d groups", shell_command_count(), count_groups());
    format(add("Files"), TEXT_MAX, "%d in the RAM disk", count_nodes(fs_root) - 1);
    format(add("Display"), TEXT_MAX, "%dx%d, theme %s, font %s", vga_cols(), vga_rows(),
           palette_current()->name, font_name(font_current()));
    format(add("Security"), TEXT_MAX, "%d attacks, %d defences",
           registry_attack_count(), registry_defence_count());
    add(0);                                                 // the colour blocks
}

/* ---------- Printing ---------- */

static void print_info(int i) {
    info_t *line = &lines[i];

    if (i == 0) {
        const user_t *me = session_user();          // who is logged in
        ui_color(UI_GOOD);
        vga_print_str(me ? me->name : "?");
        ui_reset();
        vga_print_str("@");
        ui_color(UI_TITLE);
        vga_print_str(hostname_get());
    } else if (i == 1) {
        ui_color(UI_DIM);
        vga_print_str("\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4\xC4");     // ─────────
    } else if (!line->label) {
        for (int c = 0; c < 16; c++) {                          // the 16 colours
            vga_set_color(c, c);
            vga_print_str("  ");
        }
    } else {
        ui_color(UI_ACCENT);
        ui_cell(line->label, LABEL_WIDTH);
        ui_reset();
        vga_print_str(line->text);
        if (strcmp(line->label, "Memory") == 0)
            ui_bar(pmm_get_used_blocks(), pmm_get_max_blocks(), 10);
    }
    ui_reset();
}

void cmd_uzfetch(const char *args) {
    (void)args;
    gather();

    int rows = line_count > LOGO_FLAG_ROWS ? line_count : LOGO_FLAG_ROWS;
    vga_print_str("\n");
    for (int r = 0; r < rows; r++) {
        vga_print_str(" ");
        if (r < LOGO_FLAG_ROWS)
            logo_print_flag_row(r);
        else
            for (int i = 0; i < LOGO_FLAG_WIDTH; i++)
                vga_put_char(' ');
        vga_print_str(GAP);
        if (r < line_count)
            print_info(r);
        vga_print_str("\n");
    }
    vga_print_str("\n");
}
