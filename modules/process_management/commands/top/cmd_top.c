/*
 * cmd_top.c - `top`: a live, full-screen view of the processes.
 *
 *   UzOS top   up 0:05:12   5 processes                       q = quit
 *   Memory ████░░░░░░░░░░░░░░░░   9 / 127 MB  (7%)
 *   CPU    █░░░░░░░░░░░░░░░░░░░   4% busy  (ring 3 0%, ring 0 4%)
 *     PID  STATE     PRI  CPU%  UID  NAME
 *       2  RUNNING     4    3%    0  kernel_main          system
 *
 * It refreshes every second until 'q'. While top runs the shell is not
 * reading the keyboard (it is busy running us), so top reads the keys
 * itself. It borrows the whole screen (vga_fullscreen); when it quits
 * the old screen comes back.
 *
 * Uses: the process table (core/process), pmm_* (memory_management),
 *       cpu_load (cpu_management/core/accounting),
 *       timer_uptime_ms, vga_fullscreen / vga_write_at, keyboard_pop_key
 *       (device_management), sys_sleep (syscalls/sleep) so waiting costs
 *       no CPU, def_safe_text (ui defence safe_text) for the names.
 */

#include "proc_commands.h"
#include "process.h"
#include "pmm.h"
#include "cpu_account.h"
#include "timer.h"
#include "sys_sleep.h"
#include "keyboard.h"
#include "vga.h"
#include "format.h"
#include "string.h"
#include "def_safe_text.h"

#define REFRESH_MS 1000
#define POLL_MS    100      // check for 'q' this often, so it feels instant
#define BAR_WIDTH  20
#define NAME_WIDTH 20
#define FIRST_ROW  4        // where the table starts
#define KB_PER_FRAME (PAGE_SIZE / 1024)

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
        case PROCESS_STATE_RUNNING:  return VGA_COLOR_LIGHT_GREEN;
        case PROCESS_STATE_SLEEPING: return VGA_COLOR_LIGHT_BLUE;
        case PROCESS_STATE_ZOMBIE:   return VGA_COLOR_LIGHT_RED;
        default:                     return VGA_COLOR_WHITE;
    }
}

// Write 'text' at row y from column x, and blank the rest of the row.
static void row_text(int x, int y, const char *text, enum vga_color fg, enum vga_color bg) {
    vga_write_at(x, y, text, fg, bg);
    for (int i = x + (int)strlen(text); i < vga_cols(); i++)
        vga_put_at(i, y, ' ', fg, bg);
}

// Read every key typed since the last check. 1 if one of them was 'q'.
static int quit_pressed(void) {
    char c;
    while ((c = keyboard_pop_key()))
        if (c == 'q' || c == 'Q')
            return 1;
    return 0;
}

// "Label ████░░░░" at row y: 'part' of 'whole' filled.
static void draw_bar(int y, const char *label, uint32_t part, uint32_t whole) {
    int filled = whole ? (int)((uint64_t)part * BAR_WIDTH / whole) : 0;
    if (part && !filled)
        filled = 1;
    vga_write_at(1, y, label, VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    for (int i = 0; i < BAR_WIDTH; i++)
        vga_put_at(8 + i, y, i < filled ? '\xDB' : '\xB0',
                   i < filled ? VGA_COLOR_LIGHT_GREEN : VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
}

static void draw_header(int processes) {
    char text[96];
    uint32_t s = timer_uptime_ms() / 1000;

    format(text, sizeof(text), " UzOS top   up %u:%02u:%02u   %d processes",
           s / 3600, s / 60 % 60, s % 60, processes);
    row_text(0, 0, text, VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    vga_write_at(vga_cols() - 10, 0, "q = quit", VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLUE);

    uint32_t used = pmm_get_used_blocks(), total = pmm_get_max_blocks();
    draw_bar(1, "Memory ", used, total);
    format(text, sizeof(text), "  %u / %u MB  (%u%%)",
           used * KB_PER_FRAME / 1024, total * KB_PER_FRAME / 1024,
           total ? used * 100 / total : 0);
    row_text(8 + BAR_WIDTH, 1, text, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    const cpu_load_t *load = cpu_load();
    draw_bar(2, "CPU    ", load->busy, 100);
    format(text, sizeof(text), "  %u%% busy  (ring 3 %u%%, ring 0 %u%%)",
           load->busy, load->user, load->kernel);
    row_text(8 + BAR_WIDTH, 2, text, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    row_text(0, 3, "   PID  STATE     PRI  CPU%  UID  NAME", VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN);
}

static void draw(void) {
    process_t *table = process_get_table();
    int y = FIRST_ROW, count = 0;

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state == PROCESS_STATE_FREE)
            continue;
        count++;
        if (y >= vga_rows())
            continue;                           // more than fit: count them only

        char text[96], name[NAME_WIDTH + 1];
        def_safe_text(p->name, name, NAME_WIDTH);
        format(text, sizeof(text), "  %4u", p->pid);
        row_text(0, y, text, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_write_at(8, y, state_name(p->state), state_color(p->state), VGA_COLOR_BLACK);
        format(text, sizeof(text), "%3d  %3u%%  %3u  %s",
               p->priority, p->cpu_percent, p->uid, name);
        row_text(18, y, text, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        if (p->is_system)
            vga_write_at(34 + NAME_WIDTH + 1, y, "system", VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
        y++;
    }
    for (; y < vga_rows(); y++)                 // rows of processes that are gone
        row_text(0, y, "", VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    draw_header(count);
}

void cmd_top(const char *args) {
    (void)args;
    int quit = 0;

    vga_fullscreen(1);
    while (!quit) {
        draw();
        for (int waited = 0; waited < REFRESH_MS && !quit; waited += POLL_MS) {
            sys_sleep(POLL_MS);
            quit = quit_pressed();
        }
    }
    vga_fullscreen(0);                          // the old screen comes back
}
