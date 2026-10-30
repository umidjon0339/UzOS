/*
 * cmd_top.c - `top`: a live, full-screen view of the CPU and the processes.
 *
 *   top - 19:19:03 up 5 min,  1 user,  load average: 0.00, 0.01, 0.05
 *   Tasks:   6 total,   1 running,   5 sleeping,   0 stopped,   0 zombie
 *   %Cpu(s):  1.0 us,  3.0 sy,  0.0 ni, 96.0 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st
 *   MiB Mem :    127.0 total,    118.2 free,      8.8 used,      0.0 buff/cache
 *   MiB Swap:      0.0 total,      0.0 free,      0.0 used.    118.2 avail Mem
 *
 *       PID USER      PR  NI    VIRT    RES  S  %CPU  %MEM      TIME+ COMMAND
 *         2 root      20   0       4      4  R   3.0   0.0    0:00.31 kernel_main
 *
 *   keys:  q quit   P sort by CPU   N by PID   T by CPU time   1 per-CPU line
 *   top -b -n 2      batch: print twice to the console, no full screen
 *   top -d 2         refresh every 2 s (default 3)
 *   top -p 7         only PID 7
 *
 * PR is the kernel's priority: 20 + nice for normal processes, "rt" or
 * -1 - rt_priority for real-time ones. The CPU line is the last full
 * second of core/accounting.
 *
 * While top runs the shell is not reading the keyboard (it is busy
 * running us), so top reads the keys itself. It borrows the whole
 * screen (vga_fullscreen); when it quits the old screen comes back.
 *
 * Uses: cpu_load (core/accounting), loadavg_* (core/loadavg),
 *       policy_is_realtime (core/policy), the process table
 *       (process_management), pmm_* (memory_management), users_name
 *       (user_management), timer_* / rtc_read / vga_* / keyboard_pop_key
 *       (device_management), sys_sleep_interruptible (process_management),
 *       def_safe_text (ui defence safe_text), format / string (lib).
 */

#include "modules/cpu_management/commands/top/cmd_top.h"            // this command
#include "modules/cpu_management/core/accounting/cpu_account.h"     // cpu_load
#include "modules/cpu_management/core/loadavg/loadavg.h"            // loadavg_*
#include "modules/cpu_management/core/policy/policy.h"              // policy_is_realtime
#include "modules/process_management/core/process/process.h"        // process table
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/memory_management/core/pmm/pmm.h"                 // pmm_get_*_blocks
#include "modules/user_management/core/users/users.h"               // users_name
#include "modules/device_management/timer/timer.h"                  // timer_*
#include "modules/device_management/rtc/rtc.h"                      // rtc_read
#include "modules/device_management/keyboard/keyboard.h"            // keyboard_pop_key
#include "modules/device_management/vga/vga.h"                      // vga_*
#include "modules/ui/security/defence/safe_text/def_safe_text.h"    // def_safe_text
#include "modules/ui/core/style/ui.h"                               // ui_* (batch mode)
#include "lib/args/args.h"                                          // arg_next
#include "lib/format/format.h"                                      // format
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strlen, strcmp

#define POLL_MS     100         // look for a key this often
#define FIRST_ROW   7           // where the process rows start
#define NAME_WIDTH  15
#define KB_PER_PAGE (PAGE_SIZE / 1024)
#define STACK_KB    (PROCESS_STACK_SIZE / 1024)

typedef enum { SORT_CPU, SORT_PID, SORT_TIME } sort_t;

typedef struct {
    int batch;                  // -b: print to the console instead
    uint32_t delay_ms;          // -d
    int iterations;             // -n (0 = until q)
    uint32_t only_pid;          // -p (0 = all)
    sort_t sort;
    int per_cpu;                // '1': "%Cpu0" instead of "%Cpu(s)"
} top_options_t;

/* ---------- Text lines ---------- */

// One screen line: drawn at row y (full screen) or printed (batch).
static void line(const top_options_t *o, int y, const char *text, enum vga_color fg,
                 enum vga_color bg) {
    if (o->batch) {
        ui_printf("%s\n", text);
        return;
    }
    vga_write_at(0, y, text, fg, bg);
    for (int x = strlen(text); x < vga_cols(); x++)
        vga_put_at(x, y, ' ', fg, bg);
}

// "118.2": KiB as MiB with one decimal.
static void mib(uint32_t kb, char *out, int size) {
    format(out, size, "%u.%u", kb / 1024, kb % 1024 * 10 / 1024);
}

static char state_letter(const process_t *p) {
    switch (p->state) {
        case PROCESS_STATE_RUNNING:
        case PROCESS_STATE_READY:    return 'R';
        case PROCESS_STATE_SLEEPING: return 'S';
        case PROCESS_STATE_ZOMBIE:   return 'Z';
        case PROCESS_STATE_STOPPED:  return 'T';
        default:                     return '?';
    }
}

static void header(const top_options_t *o, const process_t *table) {
    char text[100], a[12], b[12], c[12], d[12];
    uint32_t s = timer_uptime_ms() / 1000, load[3];
    rtc_time_t now;
    int have_clock = rtc_read(&now);

    loadavg_get(load);
    loadavg_format(load[0], a, sizeof(a));
    loadavg_format(load[1], b, sizeof(b));
    loadavg_format(load[2], c, sizeof(c));
    format(text, sizeof(text), "top - %02d:%02d:%02d up %u:%02u,  1 user,  load average: %s, %s, %s",
           have_clock ? now.hour : 0, have_clock ? now.minute : 0, have_clock ? now.second : 0,
           s / 3600, s / 60 % 60, a, b, c);
    line(o, 0, text, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    int total = 0, running = 0, sleeping = 0, zombie = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (table[i].state == PROCESS_STATE_FREE) continue;
        total++;
        running  += table[i].state == PROCESS_STATE_RUNNING || table[i].state == PROCESS_STATE_READY;
        sleeping += table[i].state == PROCESS_STATE_SLEEPING;
        zombie   += table[i].state == PROCESS_STATE_ZOMBIE;
    }
    format(text, sizeof(text), "Tasks: %3d total, %3d running, %3d sleeping,   0 stopped, %3d zombie",
           total, running, sleeping, zombie);
    line(o, 1, text, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    const cpu_load_t *l = cpu_load();
    format(text, sizeof(text), "%s%3u.0 us,%3u.0 sy,%3u.0 ni,%3u.0 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st",
           o->per_cpu ? "%Cpu0  :" : "%Cpu(s):", l->user, l->kernel, l->nice, l->idle);
    line(o, 2, text, VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    uint32_t total_kb = pmm_get_max_blocks() * KB_PER_PAGE;
    uint32_t free_kb  = pmm_get_free_blocks() * KB_PER_PAGE;
    mib(total_kb, a, sizeof(a));
    mib(free_kb, b, sizeof(b));
    mib(total_kb - free_kb, c, sizeof(c));
    format(text, sizeof(text), "MiB Mem : %8s total, %8s free, %8s used,      0.0 buff/cache", a, b, c);
    line(o, 3, text, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    mib(free_kb, d, sizeof(d));
    format(text, sizeof(text), "MiB Swap:      0.0 total,      0.0 free,      0.0 used. %8s avail Mem", d);
    line(o, 4, text, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    line(o, 5, "", VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    line(o, 6, "    PID USER      PR  NI    VIRT    RES  S  %CPU  %MEM      TIME+ COMMAND",
         VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
}

static uint32_t key_of(const top_options_t *o, const process_t *p) {
    if (o->sort == SORT_PID)  return 0xFFFFFFFF - p->pid;      // smallest pid first
    if (o->sort == SORT_TIME) return p->user_ticks + p->kernel_ticks;
    return p->cpu_percent * 0x10000 + (p->user_ticks + p->kernel_ticks) % 0x10000;
}

static void process_rows(const top_options_t *o, process_t *table) {
    process_t *rows[MAX_PROCESSES];
    int count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE &&
            (!o->only_pid || table[i].pid == o->only_pid))
            rows[count++] = &table[i];

    for (int i = 1; i < count; i++) {           // biggest key first
        process_t *p = rows[i];
        int j = i - 1;
        while (j >= 0 && key_of(o, rows[j]) < key_of(o, p)) {
            rows[j + 1] = rows[j];
            j--;
        }
        rows[j + 1] = p;
    }

    uint32_t total_kb = pmm_get_max_blocks() * KB_PER_PAGE;
    int y = FIRST_ROW;
    for (int r = 0; r < count && (o->batch || y < vga_rows()); r++, y++) {
        process_t *p = rows[r];
        char text[100], user[9], name[NAME_WIDTH + 1], pr[6], ni[6];
        uint32_t cs = (p->user_ticks + p->kernel_ticks) * (100 / (timer_frequency() ? timer_frequency() : 100));
        uint32_t mem_tenths = total_kb ? STACK_KB * 1000 / total_kb : 0;

        users_name(p->uid, user, sizeof(user));
        def_safe_text(p->name, name, NAME_WIDTH);
        if (policy_is_realtime(p->policy)) {
            format(pr, sizeof(pr), p->rt_priority == 99 ? "rt" : "%d", -1 - p->rt_priority);
            format(ni, sizeof(ni), "0");
        } else {
            format(pr, sizeof(pr), "%d", 20 + p->nice);
            format(ni, sizeof(ni), "%d", p->nice);
        }
        format(text, sizeof(text), "%7u %-8s %3s %3s %7u %6u  %c %3u.0 %3u.%u %4u:%02u.%02u %s",
               p->tgid ? p->tgid : p->pid, user, pr, ni, STACK_KB, STACK_KB, state_letter(p),
               p->cpu_percent, mem_tenths / 10, mem_tenths % 10,
               cs / 6000, cs / 100 % 60, cs % 100, name);
        line(o, y, text, p->state == PROCESS_STATE_RUNNING ? VGA_COLOR_WHITE : VGA_COLOR_LIGHT_GREY,
             VGA_COLOR_BLACK);
    }
    if (!o->batch)
        for (; y < vga_rows(); y++)             // rows of processes that are gone
            line(o, y, "", VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}

// Read every key typed since the last look. 1 = quit.
static int handle_keys(top_options_t *o) {
    char c;
    while ((c = keyboard_pop_key())) {
        if (c == 'q' || c == 'Q') return 1;
        if (c == 'P') o->sort = SORT_CPU;
        if (c == 'N') o->sort = SORT_PID;
        if (c == 'T') o->sort = SORT_TIME;
        if (c == '1') o->per_cpu = !o->per_cpu;
    }
    return 0;
}

static int parse(top_options_t *o, const char *args) {
    char word[16];
    for (args = arg_next(args, word, sizeof(word)); word[0];
         args = arg_next(args, word, sizeof(word))) {
        if (strcmp(word, "-b") == 0) {
            o->batch = 1;
        } else if (strcmp(word, "-d") == 0 || strcmp(word, "-n") == 0 || strcmp(word, "-p") == 0) {
            char value[16];
            char option = word[1];
            args = arg_next(args, value, sizeof(value));
            if (!value[0]) return 0;
            if (option == 'd') o->delay_ms = atoi(value) * 1000;
            if (option == 'n') o->iterations = atoi(value);
            if (option == 'p') o->only_pid = atoi(value);
        } else {
            return 0;
        }
    }
    if (o->delay_ms < 1000)
        o->delay_ms = 1000;
    return 1;
}

void cmd_top(const char *args) {
    top_options_t o = { 0, 3000, 0, 0, SORT_CPU, 0 };
    if (!parse(&o, args)) {
        ui_usage("top [-b] [-d seconds] [-n count] [-p pid]");
        return;
    }
    if (o.batch && o.iterations == 0)
        o.iterations = 1;

    int quit = 0;
    if (!o.batch)
        vga_fullscreen(1);
    for (int round = 0; !quit; round++) {
        process_t *table = process_get_table();
        header(&o, table);
        process_rows(&o, table);
        if (o.batch)
            vga_print_str("\n");
        if (o.iterations && round + 1 >= o.iterations)
            break;
        for (uint32_t waited = 0; waited < o.delay_ms && !quit; waited += POLL_MS) {
            int interrupted = !sys_sleep_interruptible(POLL_MS);
            quit = interrupted || (!o.batch && handle_keys(&o));
        }
    }
    if (!o.batch)
        vga_fullscreen(0);                  // the old screen comes back
}
