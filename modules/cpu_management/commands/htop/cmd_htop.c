/*
 * cmd_htop.c - `htop`: an interactive process viewer.
 *
 *     0[|||||                          5.0%]   Tasks: 6, 0 thr; 1 running
 *   Mem[||||                      9M/127M]   Load average: 0.00 0.01 0.05
 *   Swp[                             0K/0K]   Uptime: 00:05:12
 *
 *     PID USER      PRI  NI  VIRT   RES S CPU% MEM%   TIME+  Command
 *       2 root       20   0     4     4 R  3.0  0.0  0:00.31 kernel_main   <- selected
 *
 *   Up/Down PgUp/PgDn   select a process
 *   [  ]                nice - / nice + (lower nice = more CPU: root only)
 *   k                   kill the selected process
 *   P  M  T  N          sort by CPU, memory, time, PID
 *   q                   quit
 *
 * The CPU bar is split by colour as in htop: green = user programs,
 * blue = nice (low priority) programs, red = the kernel.
 * Renice and kill go through the same checks as renice and kill
 * (priority_bounds, kill_guard), so htop can do nothing you could not
 * do by typing.
 *
 * Uses: cpu_load (core/accounting), loadavg_* (core/loadavg),
 *       priority_set_nice / NICE_* (core/priority), policy_is_realtime
 *       (core/policy), the process table /
 *       process_kill (process_management), pmm_* (memory_management),
 *       users_name (user_management), timer_* / vga_* / keyboard_*
 *       (device_management), sys_sleep_interruptible, def_safe_text
 *       (ui defence safe_text), format / string (lib).
 */

#include "modules/cpu_management/commands/htop/cmd_htop.h"          // this command
#include "modules/cpu_management/core/accounting/cpu_account.h"     // cpu_load
#include "modules/cpu_management/core/loadavg/loadavg.h"            // loadavg_*
#include "modules/cpu_management/core/priority/priority.h"          // priority_set_nice
#include "modules/cpu_management/core/policy/policy.h"              // policy_is_realtime
#include "modules/process_management/core/scheduler/scheduler.h"    // process table, current
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/memory_management/core/pmm/pmm.h"                 // pmm_get_*_blocks
#include "modules/user_management/core/users/users.h"               // users_name
#include "modules/device_management/timer/timer.h"                  // timer_*
#include "modules/device_management/keyboard/keyboard.h"            // keyboard_pop_key, KEY_*
#include "modules/device_management/vga/vga.h"                      // vga_*
#include "modules/ui/security/defence/safe_text/def_safe_text.h"    // def_safe_text
#include "lib/format/format.h"                                      // format
#include "lib/string/string.h"                                      // strlen

#define REFRESH_MS  1000
#define POLL_MS     50
#define BAR_START   5           // first column inside "  0[" ... "]"
#define BAR_END     39          // the ']'
#define RIGHT_COL   42          // where "Tasks:", "Load average:" start
#define LIST_ROW    5           // first process row
#define KB_PER_PAGE (PAGE_SIZE / 1024)
#define STACK_KB    (PROCESS_STACK_SIZE / 1024)
#define NAME_WIDTH  20

enum { SORT_CPU, SORT_MEM, SORT_TIME, SORT_PID };

typedef struct {
    uint32_t selected_pid;      // stays selected while it exists
    int      sort;
    char     message[48];       // shown in the footer after an action
} htop_state_t;

/* ---------- Drawing helpers ---------- */

static void text_at(int x, int y, const char *text, enum vga_color fg, enum vga_color bg) {
    vga_write_at(x, y, text, fg, bg);
}

static void clear_row(int y, int from) {
    for (int x = from; x < vga_cols(); x++)
        vga_put_at(x, y, ' ', VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

// "  0[|||||   5.0%]": parts[] are filled in order with their colours.
static void meter(int y, const char *label, const uint32_t *parts, const enum vga_color *colors,
                  int count, uint32_t whole, const char *right_text) {
    int width = BAR_END - BAR_START;
    int x = BAR_START;

    text_at(BAR_START - 1 - strlen(label), y, label, VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    text_at(BAR_START - 1, y, "[", VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    for (int i = 0; i < count; i++) {
        int n = whole ? (int)((uint64_t)parts[i] * width / whole) : 0;
        for (int k = 0; k < n && x < BAR_END; k++)
            vga_put_at(x++, y, '|', colors[i], VGA_COLOR_BLACK);
    }
    while (x < BAR_END)
        vga_put_at(x++, y, ' ', VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    text_at(BAR_END - strlen(right_text), y, right_text, VGA_COLOR_DARK_GREY, VGA_COLOR_BLACK);
    text_at(BAR_END, y, "]", VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

static void label_value(int y, const char *label, const char *value) {
    clear_row(y, RIGHT_COL);
    text_at(RIGHT_COL, y, label, VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    text_at(RIGHT_COL + strlen(label), y, value, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

/* ---------- The header ---------- */

static void draw_header(process_t *table) {
    char text[64], a[8], b[8], c[8];
    const cpu_load_t *l = cpu_load();

    uint32_t cpu_parts[3] = { l->user, l->nice, l->kernel };
    enum vga_color cpu_colors[3] = { VGA_COLOR_LIGHT_GREEN, VGA_COLOR_LIGHT_BLUE, VGA_COLOR_LIGHT_RED };
    format(text, sizeof(text), "%u.0%%", l->busy);
    meter(0, "0", cpu_parts, cpu_colors, 3, 100, text);

    uint32_t used = pmm_get_used_blocks(), total = pmm_get_max_blocks();
    uint32_t mem_parts[1] = { used };
    enum vga_color mem_colors[1] = { VGA_COLOR_LIGHT_GREEN };
    format(text, sizeof(text), "%uM/%uM", used * KB_PER_PAGE / 1024, total * KB_PER_PAGE / 1024);
    meter(1, "Mem", mem_parts, mem_colors, 1, total, text);
    meter(2, "Swp", mem_parts, mem_colors, 0, 1, "0K/0K");

    int groups = 0, threads = 0, running = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state == PROCESS_STATE_FREE) continue;
        if (p->tgid && p->tgid != p->pid) threads++; else groups++;
        running += p->state == PROCESS_STATE_RUNNING || p->state == PROCESS_STATE_READY;
    }
    format(text, sizeof(text), "%d, %d thr; %d running", groups, threads, running);
    label_value(0, "Tasks: ", text);

    uint32_t load[3];
    loadavg_get(load);
    loadavg_format(load[0], a, sizeof(a));
    loadavg_format(load[1], b, sizeof(b));
    loadavg_format(load[2], c, sizeof(c));
    format(text, sizeof(text), "%s %s %s", a, b, c);
    label_value(1, "Load average: ", text);

    uint32_t s = timer_uptime_ms() / 1000;
    format(text, sizeof(text), "%02u:%02u:%02u", s / 3600, s / 60 % 60, s % 60);
    label_value(2, "Uptime: ", text);
    clear_row(3, 0);
}

/* ---------- The process list ---------- */

static uint32_t sort_key(const htop_state_t *st, const process_t *p) {
    switch (st->sort) {
        case SORT_PID:  return 0xFFFFFFFF - p->pid;
        case SORT_TIME: return p->user_ticks + p->kernel_ticks;
        case SORT_MEM:  return 0xFFFFFFFF - p->pid;   // every process has one stack page
        default:        return p->cpu_percent * 0x10000 + (p->user_ticks + p->kernel_ticks) % 0x10000;
    }
}

static int collect(const htop_state_t *st, process_t **rows) {
    process_t *table = process_get_table();
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE)
            rows[count++] = &table[i];
    for (int i = 1; i < count; i++) {
        process_t *p = rows[i];
        int j = i - 1;
        while (j >= 0 && sort_key(st, rows[j]) < sort_key(st, p)) {
            rows[j + 1] = rows[j];
            j--;
        }
        rows[j + 1] = p;
    }
    return count;
}

static void draw_list(htop_state_t *st, process_t **rows, int count) {
    int visible = vga_rows() - LIST_ROW - 1;        // the last row is the footer
    int selected = 0;
    for (int i = 0; i < count; i++)
        if (rows[i]->pid == st->selected_pid)
            selected = i;
    if (count)
        st->selected_pid = rows[selected]->pid;
    int first = selected >= visible ? selected - visible + 1 : 0;

    char text[96];
    format(text, sizeof(text), "%5s %-8s %4s %3s %5s %5s %s %4s %4s %8s  %s",
           "PID", "USER", "PRI", "NI", "VIRT", "RES", "S", "CPU%", "MEM%", "TIME+", "Command");
    text_at(0, LIST_ROW - 1, text, VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN);
    for (int x = strlen(text); x < vga_cols(); x++)
        vga_put_at(x, LIST_ROW - 1, ' ', VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREEN);

    uint32_t total_kb = pmm_get_max_blocks() * KB_PER_PAGE;
    for (int r = 0; r < visible; r++) {
        int y = LIST_ROW + r;
        int i = first + r;
        if (i >= count) {
            clear_row(y, 0);
            continue;
        }
        process_t *p = rows[i];
        char user[9], name[NAME_WIDTH + 1], pri[6], ni[6];
        uint32_t cs = (p->user_ticks + p->kernel_ticks) * (100 / (timer_frequency() ? timer_frequency() : 100));
        uint32_t mem = total_kb ? STACK_KB * 1000 / total_kb : 0;
        char state = p->state == PROCESS_STATE_SLEEPING ? 'S' :
                     p->state == PROCESS_STATE_ZOMBIE ? 'Z' : 'R';

        users_name(p->uid, user, sizeof(user));
        def_safe_text(p->name, name, NAME_WIDTH);
        if (policy_is_realtime(p->policy)) {
            format(pri, sizeof(pri), "RT");
            format(ni, sizeof(ni), "0");
        } else {
            format(pri, sizeof(pri), "%d", 20 + p->nice);
            format(ni, sizeof(ni), "%d", p->nice);
        }
        format(text, sizeof(text), "%5u %-8s %4s %3s %5u %5u %c %2u.0 %2u.%u %2u:%02u.%02u  %s",
               p->pid, user, pri, ni, STACK_KB, STACK_KB, state, p->cpu_percent,
               mem / 10, mem % 10, cs / 6000, cs / 100 % 60, cs % 100, name);

        int is_selected = i == selected;
        enum vga_color fg = is_selected ? VGA_COLOR_BLACK : VGA_COLOR_LIGHT_GREY;
        enum vga_color bg = is_selected ? VGA_COLOR_LIGHT_CYAN : VGA_COLOR_BLACK;
        text_at(0, y, text, fg, bg);
        for (int x = strlen(text); x < vga_cols(); x++)
            vga_put_at(x, y, ' ', fg, bg);
    }
}

static void draw_footer(const htop_state_t *st) {
    static const char *keys[][2] = {
        { "Up/Dn", "Select" }, { "[", "Nice-" }, { "]", "Nice+" }, { "k", "Kill" },
        { "P", "CPU" }, { "T", "Time" }, { "N", "PID" }, { "q", "Quit" },
    };
    int y = vga_rows() - 1, x = 0;
    for (unsigned i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
        text_at(x, y, keys[i][0], VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        x += strlen(keys[i][0]);
        text_at(x, y, keys[i][1], VGA_COLOR_BLACK, VGA_COLOR_CYAN);
        x += strlen(keys[i][1]);
        vga_put_at(x++, y, ' ', VGA_COLOR_BLACK, VGA_COLOR_CYAN);
    }
    clear_row(y, x);
    text_at(x + 1, y, st->message, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
}

/* ---------- Keys ---------- */

static void renice(htop_state_t *st, int step) {
    process_t *p = process_get_by_pid(st->selected_pid);
    if (!p)
        return;
    int nice = p->nice + step;
    if (nice < NICE_MIN || nice > NICE_MAX ||
        !priority_set_nice(scheduler_get_current_process(), p->pid, nice))
        format(st->message, sizeof(st->message), "renice %u refused", p->pid);
    else
        format(st->message, sizeof(st->message), "PID %u: nice %d", p->pid, nice);
}

static void kill_selected(htop_state_t *st) {
    uint32_t pid = st->selected_pid;
    if (process_kill(pid))
        format(st->message, sizeof(st->message), "killed %u", pid);
    else
        format(st->message, sizeof(st->message), "kill %u refused (kill_guard)", pid);
}

static void move(htop_state_t *st, process_t **rows, int count, int step) {
    for (int i = 0; i < count; i++) {
        if (rows[i]->pid != st->selected_pid)
            continue;
        int j = i + step;
        if (j < 0) j = 0;
        if (j >= count) j = count - 1;
        st->selected_pid = rows[j]->pid;
        return;
    }
}

// Handle typed keys. 1 = quit; *changed = 1 if the screen should redraw.
static int handle_keys(htop_state_t *st, int *changed) {
    process_t *rows[MAX_PROCESSES];
    int page = vga_rows() - LIST_ROW - 1;
    char c;
    while ((c = keyboard_pop_key())) {
        unsigned char key = (unsigned char)c;
        int count = collect(st, rows);
        *changed = 1;
        st->message[0] = '\0';
        if (c == 'q' || c == 'Q')  return 1;
        if (key == KEY_UP)         move(st, rows, count, -1);
        if (key == KEY_DOWN)       move(st, rows, count, 1);
        if (key == KEY_PGUP)       move(st, rows, count, -page);
        if (key == KEY_PGDN)       move(st, rows, count, page);
        if (c == '[')              renice(st, -1);
        if (c == ']')              renice(st, 1);
        if (c == 'k')              kill_selected(st);
        if (c == 'P')              st->sort = SORT_CPU;
        if (c == 'M')              st->sort = SORT_MEM;
        if (c == 'T')              st->sort = SORT_TIME;
        if (c == 'N')              st->sort = SORT_PID;
    }
    return 0;
}

void cmd_htop(const char *args) {
    (void)args;
    htop_state_t st = { 0, SORT_CPU, "" };
    process_t *rows[MAX_PROCESSES];
    int quit = 0;

    vga_fullscreen(1);
    while (!quit) {
        process_t *table = process_get_table();
        int count = collect(&st, rows);
        draw_header(table);
        draw_list(&st, rows, count);
        draw_footer(&st);

        int changed = 0;
        for (int waited = 0; waited < REFRESH_MS && !quit && !changed; waited += POLL_MS) {
            int interrupted = !sys_sleep_interruptible(POLL_MS);
            quit = interrupted || handle_keys(&st, &changed);
        }
    }
    vga_fullscreen(0);
}
