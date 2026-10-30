/*
 * statusbar.c - Build the status row and keep it fresh (the statusd daemon).
 *
 * The row is built in 'cells' from left to right; the right-hand part
 * is built separately first, so it can be placed against the right edge.
 * A wider screen (the graphics console) also gets the uptime and date.
 *
 * Uses: vga_status_* (device_management vga), rtc_read (rtc/),
 *       pmm_* (memory_management), the process table and sys_sleep
 *       (process_management), vfs_path_of (filesystem_management),
 *       timer_uptime_ms, format (lib/format).
 */

#include "statusbar.h"
#include "vga.h"
#include "glyphs.h"
#include "rtc.h"
#include "timer.h"
#include "pmm.h"
#include "process.h"
#include "vfs.h"
#include "cpu.h"
#include "format.h"
#include "sys_sleep.h"

#define REFRESH_MS  500
#define BAR_BG      VGA_COLOR_BLUE
#define SEPARATOR   "\xB3"          // │
#define FULL        '\xDB'          // █
#define LIGHT       '\xB0'          // ░
#define MEM_BAR     5

static uint32_t shell;              // whose current directory to show
static int shown = 1;

/* ---------- Building a row of cells ---------- */

typedef struct {
    uint16_t cells[VGA_MAX_COLS];
    int length;
} row_t;

static void add(row_t *r, const char *text, enum vga_color fg) {
    for (; *text && r->length < VGA_MAX_COLS; text++)
        r->cells[r->length++] = vga_cell(*text, fg, BAR_BG);
}

static void add_separator(row_t *r) {
    add(r, " " SEPARATOR " ", VGA_COLOR_LIGHT_CYAN);
}

static void add_memory_bar(row_t *r) {
    uint32_t used = pmm_get_used_blocks(), total = pmm_get_max_blocks();
    int filled = total ? (int)((uint64_t)used * MEM_BAR / total) : 0;
    if (used && !filled)
        filled = 1;
    for (int i = 0; i < MEM_BAR && r->length < VGA_MAX_COLS; i++)
        r->cells[r->length++] = vga_cell(i < filled ? FULL : LIGHT,
                                         i < filled ? VGA_COLOR_LIGHT_GREEN : VGA_COLOR_LIGHT_GREY,
                                         BAR_BG);
    char text[8];
    format(text, sizeof(text), " %u%%", total ? used * 100 / total : 0);
    add(r, text, VGA_COLOR_WHITE);
}

static int count_processes(void) {
    process_t *table = process_get_table();
    int n = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        n += table[i].state != PROCESS_STATE_FREE;
    return n;
}

// The shell's current directory as text.
static void shell_path(char *out, int size) {
    uint32_t irq = cpu_irq_save();          // the shell may cd meanwhile
    process_t *p = process_get_by_pid(shell);
    vfs_node_t *dir = p && p->cwd ? p->cwd : fs_root;
    if (!dir || vfs_path_of(dir, out, size) != 0)
        format(out, size, "?");
    cpu_irq_restore(irq);
}

/* ---------- The whole bar ---------- */

static void build(row_t *bar, int cols) {
    static row_t right;             // static, like 'bar': too big for a 4 KiB stack
    right.length = 0;
    char text[48];
    int wide = cols > 80;

    // Right side: processes, memory, (uptime), clock.
    format(text, sizeof(text), GLYPH_GEAR " %d", count_processes());
    add(&right, text, VGA_COLOR_WHITE);
    add_separator(&right);
    add(&right, GLYPH_CHIP " ", VGA_COLOR_WHITE);
    add_memory_bar(&right);
    if (wide) {
        uint32_t s = timer_uptime_ms() / 1000;
        format(text, sizeof(text), "up %u:%02u:%02u", s / 3600, s / 60 % 60, s % 60);
        add_separator(&right);
        add(&right, text, VGA_COLOR_WHITE);
    }
    add_separator(&right);
    rtc_time_t now;
    if (!rtc_read(&now))
        format(text, sizeof(text), GLYPH_CLOCK " --:--:--");
    else if (wide)
        format(text, sizeof(text), GLYPH_CLOCK " %s %d %s %02d:%02d:%02d",
               rtc_weekday_name(now.weekday), now.day, rtc_month_name(now.month),
               now.hour, now.minute, now.second);
    else
        format(text, sizeof(text), GLYPH_CLOCK " %02d:%02d:%02d", now.hour, now.minute, now.second);
    add(&right, text, VGA_COLOR_WHITE);
    add(&right, " ", VGA_COLOR_WHITE);

    // Left side: the name, then the path in whatever room is left.
    bar->length = 0;
    add(bar, " " GLYPH_CRESCENT " ", VGA_COLOR_WHITE);
    add(bar, "UzOS", VGA_COLOR_LIGHT_CYAN);
    add_separator(bar);

    char path[FS_PATH_MAX];
    shell_path(path, sizeof(path));
    int room = cols - right.length - bar->length - 1;
    int n = 0;
    while (path[n]) n++;
    if (n > room && room > 1) {             // too long: keep the end, "…/deep/dir"
        bar->cells[bar->length++] = vga_cell('\xAF', VGA_COLOR_LIGHT_GREY, BAR_BG);
        add(bar, path + n - (room - 1), VGA_COLOR_LIGHT_BROWN);
    } else if (room > 1) {
        add(bar, path, VGA_COLOR_LIGHT_BROWN);
    }

    while (bar->length < cols - right.length)
        add(bar, " ", VGA_COLOR_WHITE);
    for (int i = 0; i < right.length && bar->length < VGA_MAX_COLS; i++)
        bar->cells[bar->length++] = right.cells[i];
}

void statusbar_update(void) {
    if (!shown)
        return;
    static row_t bar;               // static: ~480 bytes is a lot for a 4 KiB stack
    uint32_t irq = cpu_irq_save();  // statusd and a command may both call this
    build(&bar, vga_cols());
    vga_status_set(bar.cells);
    cpu_irq_restore(irq);
}

/* ---------- The daemon ---------- */

static void statusd(void) {
    for (;;) {
        statusbar_update();
        sys_sleep(REFRESH_MS);
    }
}

void statusbar_start(uint32_t shell_pid) {
    shell = shell_pid;
    vga_status_enable(1);
    statusbar_update();

    int pid = process_create("statusd", statusd, 0);
    process_t *p = pid < 0 ? 0 : process_get_by_pid(pid);
    if (p)
        p->is_system = 1;           // part of the OS: kill_guard protects it
}

void statusbar_show(int on) {
    shown = on;
    vga_status_enable(on);
    statusbar_update();
}

int statusbar_shown(void) {
    return shown;
}
