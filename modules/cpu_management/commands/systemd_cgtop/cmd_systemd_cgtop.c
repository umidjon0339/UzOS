/*
 * cmd_systemd_cgtop.c - `systemd-cgtop`: CPU and memory per group of processes.
 *
 *   Control Group                          Tasks    %CPU     Memory
 *   /                                          6     4.0      24.0K
 *   /system.slice                              2     4.0       8.0K
 *   /user.slice                                3     0.0      12.0K
 *   /user.slice/user-0.slice                   2     0.0       8.0K
 *   /user.slice/user-1000.slice                1     0.0       4.0K
 *
 * Linux puts every process in a CONTROL GROUP (cgroup), a tree: services
 * under /system.slice, each user's login under /user.slice/user-UID.slice.
 * Limits and accounting then apply to a whole group at once.
 *
 * UzOS has no cgroups, but the same grouping can be computed: system
 * processes (is_system) form /system.slice, every other process belongs
 * to its user's slice. The idle task is not counted (idle is not use).
 * Memory = each process's 4 KiB kernel stack.
 *
 *   systemd-cgtop          live, full screen, every second; q quits
 *   systemd-cgtop -n 2     print twice to the console, then stop
 *
 * Uses: the process table and the idle task (process_management),
 *       vga_* / keyboard_pop_key / timer (device_management),
 *       sys_sleep_interruptible, ui_* (ui), format / arg_next / atoi (lib).
 */

#include "modules/cpu_management/commands/systemd_cgtop/cmd_systemd_cgtop.h"
#include "modules/process_management/core/scheduler/scheduler.h"    // process table, idle task
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/device_management/keyboard/keyboard.h"            // keyboard_pop_key
#include "modules/device_management/vga/vga.h"                      // vga_*
#include "modules/ui/core/style/ui.h"                               // ui_*
#include "lib/args/args.h"                                          // arg_next
#include "lib/format/format.h"                                      // format
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp, strlen

#define MAX_GROUPS  12
#define REFRESH_MS  1000
#define POLL_MS     100
#define STACK_KB    (PROCESS_STACK_SIZE / 1024)

typedef struct {
    char     path[40];
    int      tasks;
    uint32_t cpu;           // percent, summed
} group_t;

static group_t *find_or_add(group_t *g, int *count, const char *path) {
    for (int i = 0; i < *count; i++)
        if (strcmp(g[i].path, path) == 0)
            return &g[i];
    if (*count >= MAX_GROUPS)
        return 0;
    group_t *n = &g[(*count)++];
    format(n->path, sizeof(n->path), "%s", path);
    n->tasks = 0;
    n->cpu = 0;
    return n;
}

static void add(group_t *g, const process_t *p) {
    if (g) {
        g->tasks++;
        g->cpu += p->cpu_percent;
    }
}

// Group every process (but the idle task) into its slices.
static int collect(group_t *g) {
    int count = 0;
    process_t *table = process_get_table();
    find_or_add(g, &count, "/");
    find_or_add(g, &count, "/system.slice");
    find_or_add(g, &count, "/user.slice");

    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state == PROCESS_STATE_FREE || p == scheduler_idle_process())
            continue;
        add(&g[0], p);
        if (p->is_system) {
            add(&g[1], p);
        } else {
            char path[40];
            format(path, sizeof(path), "/user.slice/user-%u.slice", p->uid);
            add(&g[2], p);
            add(find_or_add(g, &count, path), p);
        }
    }
    return count;
}

static void group_line(const group_t *g, char *out, int size) {
    uint32_t kb = g->tasks * STACK_KB;
    format(out, size, "%-38s %5d %5u.0 %7u.0K", g->path, g->tasks, g->cpu, kb);
}

static const char *HEADER = "Control Group                          Tasks    %CPU     Memory";

static void print_batch(void) {
    group_t g[MAX_GROUPS];
    int count = collect(g);
    char text[80];
    ui_color(UI_TITLE);
    ui_printf("%s\n", HEADER);
    ui_reset();
    for (int i = 0; i < count; i++) {
        group_line(&g[i], text, sizeof(text));
        ui_printf("%s\n", text);
    }
    vga_print_str("\n");
}

static void draw_screen(void) {
    group_t g[MAX_GROUPS];
    int count = collect(g);
    char text[80];

    for (int y = 0; y < vga_rows(); y++) {
        enum vga_color fg = VGA_COLOR_LIGHT_GREY, bg = VGA_COLOR_BLACK;
        text[0] = '\0';
        if (y == 0) {
            format(text, sizeof(text), "%s", HEADER);
            fg = VGA_COLOR_BLACK;
            bg = VGA_COLOR_LIGHT_GREY;
        } else if (y - 1 < count) {
            group_line(&g[y - 1], text, sizeof(text));
        } else if (y == vga_rows() - 1) {
            format(text, sizeof(text), "q = quit   (UzOS: slices computed from is_system and uid)");
            fg = VGA_COLOR_DARK_GREY;
        }
        vga_write_at(0, y, text, fg, bg);
        for (int x = strlen(text); x < vga_cols(); x++)
            vga_put_at(x, y, ' ', fg, bg);
    }
}

static int quit_pressed(void) {
    char c;
    while ((c = keyboard_pop_key()))
        if (c == 'q' || c == 'Q')
            return 1;
    return 0;
}

void cmd_systemd_cgtop(const char *args) {
    char word[16];
    int iterations = 0;

    for (args = arg_next(args, word, sizeof(word)); word[0];
         args = arg_next(args, word, sizeof(word))) {
        if (strcmp(word, "-n") == 0) {
            args = arg_next(args, word, sizeof(word));
            iterations = atoi(word);
        } else if (strcmp(word, "-b") != 0) {
            ui_usage("systemd-cgtop [-n count]");
            return;
        }
    }

    if (iterations > 0) {                       // batch: print, wait, print
        for (int i = 0; i < iterations; i++) {
            print_batch();
            if (i + 1 < iterations && !sys_sleep_interruptible(REFRESH_MS))
                break;
        }
        return;
    }

    int quit = 0;
    vga_fullscreen(1);
    while (!quit) {
        draw_screen();
        for (int waited = 0; waited < REFRESH_MS && !quit; waited += POLL_MS)
            quit = !sys_sleep_interruptible(POLL_MS) || quit_pressed();
    }
    vga_fullscreen(0);
}
