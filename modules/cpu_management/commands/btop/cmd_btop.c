/*
 * cmd_btop.c - `btop`: a resource monitor in boxes.
 *
 *   ┌─ cpu ─────────────────────────────────────────────┬─────────────────┐
 *   │            the last 60 seconds of CPU use         │ CPU ███░░  12%  │
 *   │       ▄█                                          │ us 2  ni 0      │
 *   │  ▄▄▄███▄▄                                         │ sy 10 id 88     │
 *   └───────────────────────────────────────────────────┴─────────────────┘
 *   ┌─ mem ───────────────┐┌─ proc ─────────────────────────────────────────┐
 *   │ Used  ██░░░░  9 MiB ││   Pid Program          User      Thr  Cpu%     │
 *   └─────────────────────┘└────────────────────────────────────────────────┘
 *
 * The graph comes from the kernel's per-second history (core/accounting):
 * one column per second, newest on the right, each column up to 12
 * steps high (6 rows of half blocks). Refreshes every second; q quits.
 *
 * Uses: cpu_load / cpu_history (core/accounting), loadavg_* (core/loadavg),
 *       tsc_mhz (core/tsc), thread_count (in this file), the process
 *       table (process_management), pmm_* (memory_management), users_name
 *       (user_management), timer_* / vga_* / keyboard_pop_key
 *       (device_management), sys_sleep_interruptible, def_safe_text
 *       (ui defence safe_text), format / string (lib).
 */

#include "modules/cpu_management/commands/btop/cmd_btop.h"          // this command
#include "modules/cpu_management/core/accounting/cpu_account.h"     // cpu_load, cpu_history
#include "modules/cpu_management/core/loadavg/loadavg.h"            // loadavg_*
#include "modules/cpu_management/core/tsc/tsc.h"                    // tsc_mhz
#include "modules/process_management/core/process/process.h"        // process table
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/memory_management/core/pmm/pmm.h"                 // pmm_get_*_blocks
#include "modules/user_management/core/users/users.h"               // users_name
#include "modules/device_management/timer/timer.h"                  // timer_uptime_ms
#include "modules/device_management/keyboard/keyboard.h"            // keyboard_pop_key
#include "modules/device_management/vga/vga.h"                      // vga_*
#include "modules/ui/security/defence/safe_text/def_safe_text.h"    // def_safe_text
#include "lib/format/format.h"                                      // format
#include "lib/string/string.h"                                      // strlen

#define REFRESH_MS   1000
#define POLL_MS      100
#define GRAPH_ROWS   6          // the cpu box is GRAPH_ROWS + 2 high
#define GRAPH_WIDTH  60         // one column per second
#define PANEL_X      62         // the right part of the cpu box
#define MEM_WIDTH    26         // the mem box, then the proc box
#define KB_PER_PAGE  (PAGE_SIZE / 1024)

// CP437 characters (the console font has them)
#define C_H     '\xC4'          // ─
#define C_V     '\xB3'          // │
#define C_TL    '\xDA'          // ┌
#define C_TR    '\xBF'          // ┐
#define C_BL    '\xC0'          // └
#define C_BR    '\xD9'          // ┘
#define C_FULL  '\xDB'          // █
#define C_LOWER '\xDC'          // ▄
#define C_LIGHT '\xB0'          // ░

#define FRAME VGA_COLOR_DARK_GREY
#define BACK  VGA_COLOR_BLACK

/* ---------- Drawing ---------- */

// Threads alive in thread group 'tgid' (every clone(CLONE_THREAD) child
// has its parent's tgid). Only this file needs it, so it lives here.
static int thread_count(uint32_t tgid) {
    process_t *table = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].tgid == tgid && table[i].state != PROCESS_STATE_FREE &&
            table[i].state != PROCESS_STATE_ZOMBIE)
            n++;
    return n;
}


static void put(int x, int y, char c, enum vga_color fg) {
    vga_put_at(x, y, c, fg, BACK);
}

static void text(int x, int y, const char *s, enum vga_color fg) {
    vga_write_at(x, y, s, fg, BACK);
}

// A frame with " title " in its top line.
static void box(int x, int y, int w, int h, const char *title) {
    for (int i = 1; i < w - 1; i++) {
        put(x + i, y, C_H, FRAME);
        put(x + i, y + h - 1, C_H, FRAME);
    }
    for (int j = 1; j < h - 1; j++) {
        put(x, y + j, C_V, FRAME);
        put(x + w - 1, y + j, C_V, FRAME);
        for (int i = 1; i < w - 1; i++)
            put(x + i, y + j, ' ', FRAME);      // clear the inside
    }
    put(x, y, C_TL, FRAME);
    put(x + w - 1, y, C_TR, FRAME);
    put(x, y + h - 1, C_BL, FRAME);
    put(x + w - 1, y + h - 1, C_BR, FRAME);
    put(x + 2, y, ' ', FRAME);
    text(x + 3, y, title, VGA_COLOR_WHITE);
    put(x + 3 + strlen(title), y, ' ', FRAME);
}

// A small bar: 'part' of 'whole', 'width' cells. 'warn' = colour the
// far end yellow and red (for "used" bars; a "free" bar stays green).
static void bar(int x, int y, uint32_t part, uint32_t whole, int width, int warn) {
    int filled = whole ? (int)((uint64_t)part * width / whole) : 0;
    if (part && !filled)
        filled = 1;
    for (int i = 0; i < width; i++) {
        enum vga_color c = !warn || i < width / 2 ? VGA_COLOR_LIGHT_GREEN :
                           i < width * 3 / 4 ? VGA_COLOR_LIGHT_BROWN : VGA_COLOR_LIGHT_RED;
        put(x + i, y, i < filled ? C_FULL : C_LIGHT, i < filled ? c : FRAME);
    }
}

/* ---------- The cpu box ---------- */

static enum vga_color graph_color(int row_from_bottom) {
    if (row_from_bottom >= GRAPH_ROWS * 2 / 3) return VGA_COLOR_LIGHT_RED;
    if (row_from_bottom >= GRAPH_ROWS / 3)     return VGA_COLOR_LIGHT_BROWN;
    return VGA_COLOR_LIGHT_GREEN;
}

static void cpu_box(void) {
    char s[40], a[8], b[8], c[8];
    uint32_t up = timer_uptime_ms() / 1000;

    box(0, 0, vga_cols(), GRAPH_ROWS + 2, "cpu");
    format(s, sizeof(s), " up %02u:%02u:%02u ", up / 3600, up / 60 % 60, up % 60);
    text(vga_cols() - 2 - strlen(s), 0, s, VGA_COLOR_WHITE);
    for (int j = 1; j <= GRAPH_ROWS; j++)
        put(PANEL_X - 1, j, C_V, FRAME);

    // The graph: column GRAPH_WIDTH is now, column 1 is a minute ago.
    for (int col = 0; col < GRAPH_WIDTH; col++) {
        const cpu_second_t *sec = cpu_history(GRAPH_WIDTH - 1 - col);
        uint32_t busy = sec ? sec->load.busy : 0;
        int halves = (busy * GRAPH_ROWS * 2 + 99) / 100;      // 0..12, never 0 if busy
        for (int k = 0; k < GRAPH_ROWS; k++) {                // k = rows from the bottom
            char ch = halves >= 2 * (k + 1) ? C_FULL : halves == 2 * k + 1 ? C_LOWER : ' ';
            put(1 + col, GRAPH_ROWS - k, ch, graph_color(k));
        }
    }

    // The panel: totals of the last second.
    const cpu_load_t *l = cpu_load();
    text(PANEL_X, 1, "CPU", VGA_COLOR_LIGHT_CYAN);
    bar(PANEL_X + 4, 1, l->busy, 100, 8, 1);
    format(s, sizeof(s), "%3u%%", l->busy);
    text(PANEL_X + 13, 1, s, VGA_COLOR_WHITE);
    format(s, sizeof(s), "us %-3u ni %u", l->user, l->nice);
    text(PANEL_X, 2, s, VGA_COLOR_LIGHT_GREY);
    format(s, sizeof(s), "sy %-3u id %u", l->kernel, l->idle);
    text(PANEL_X, 3, s, VGA_COLOR_LIGHT_GREY);

    uint32_t load[3];
    loadavg_get(load);
    loadavg_format(load[0], a, sizeof(a));
    loadavg_format(load[1], b, sizeof(b));
    loadavg_format(load[2], c, sizeof(c));
    text(PANEL_X, 4, "Load", VGA_COLOR_LIGHT_CYAN);
    format(s, sizeof(s), "%s %s", a, b);
    text(PANEL_X + 5, 4, s, VGA_COLOR_WHITE);
    text(PANEL_X + 5, 5, c, VGA_COLOR_WHITE);
    format(s, sizeof(s), "%u MHz", tsc_mhz());
    text(PANEL_X, 6, s, VGA_COLOR_DARK_GREY);
}

/* ---------- The mem box ---------- */

static void mem_box(int y, int h) {
    char s[32];
    uint32_t total = pmm_get_max_blocks(), used = pmm_get_used_blocks();

    box(0, y, MEM_WIDTH, h, "mem");
    format(s, sizeof(s), "Total %8u MiB", total * KB_PER_PAGE / 1024);
    text(2, y + 1, s, VGA_COLOR_WHITE);
    text(2, y + 3, "Used", VGA_COLOR_LIGHT_CYAN);
    bar(7, y + 3, used, total, 9, 1);
    format(s, sizeof(s), "%3uM", used * KB_PER_PAGE / 1024);
    text(MEM_WIDTH - 6, y + 3, s, VGA_COLOR_WHITE);
    text(2, y + 4, "Free", VGA_COLOR_LIGHT_CYAN);
    bar(7, y + 4, total - used, total, 9, 0);
    format(s, sizeof(s), "%3uM", (total - used) * KB_PER_PAGE / 1024);
    text(MEM_WIDTH - 6, y + 4, s, VGA_COLOR_WHITE);
    text(2, y + 6, "Swap  none", VGA_COLOR_DARK_GREY);
}

/* ---------- The proc box ---------- */

static void proc_box(int y, int h) {
    int x = MEM_WIDTH, w = vga_cols() - MEM_WIDTH;
    process_t *table = process_get_table();
    process_t *rows[MAX_PROCESSES];
    int count = 0;

    box(x, y, w, h, "proc");
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE)
            rows[count++] = &table[i];
    for (int i = 1; i < count; i++) {               // most CPU first
        process_t *p = rows[i];
        int j = i - 1;
        while (j >= 0 && rows[j]->cpu_percent < p->cpu_percent) {
            rows[j + 1] = rows[j];
            j--;
        }
        rows[j + 1] = p;
    }

    text(x + 2, y + 1, "  Pid Program          User     Thr  Cpu%", VGA_COLOR_LIGHT_CYAN);
    for (int r = 0; r < count && r < h - 3; r++) {
        process_t *p = rows[r];
        char line[64], name[17], user[9];
        def_safe_text(p->name, name, 16);
        users_name(p->uid, user, sizeof(user));
        format(line, sizeof(line), "%5u %-16s %-8s %3d %4u%%", p->pid, name, user,
               thread_count(p->tgid ? p->tgid : p->pid), p->cpu_percent);
        text(x + 2, y + 2 + r, line, p->cpu_percent ? VGA_COLOR_WHITE : VGA_COLOR_LIGHT_GREY);
        bar(x + w - 8, y + 2 + r, p->cpu_percent, 100, 5, 1);
    }
}

static int quit_pressed(void) {
    char c;
    while ((c = keyboard_pop_key()))
        if (c == 'q' || c == 'Q')
            return 1;
    return 0;
}

void cmd_btop(const char *args) {
    (void)args;
    int quit = 0;

    vga_fullscreen(1);
    while (!quit) {
        int lower_y = GRAPH_ROWS + 2, lower_h = vga_rows() - lower_y;
        cpu_box();
        mem_box(lower_y, lower_h);
        proc_box(lower_y, lower_h);
        for (int waited = 0; waited < REFRESH_MS && !quit; waited += POLL_MS)
            quit = !sys_sleep_interruptible(POLL_MS) || quit_pressed();
    }
    vga_fullscreen(0);
}
