/*
 * cmd_getconf.c - `getconf NAME` / `getconf -a`: the system's sizes and counts.
 *
 *   getconf _NPROCESSORS_ONLN     1      CPUs running
 *   getconf PAGESIZE              4096   bytes in a page
 *   getconf CLK_TCK               100    timer ticks per second
 *   getconf -a                    every name with its value
 *
 * Portable programs ask these (C: sysconf()) instead of hard-coding
 * "4096" or "8 CPUs". The values come from the parts of UzOS that set
 * them, so they are never out of date.
 *
 * Uses: affinity_cpu_count (core/affinity), pmm_* (memory_management),
 *       MAX_PROCESSES / MAX_FDS (process_management), LINE_MAX (shell),
 *       FS_NAME_MAX / FS_PATH_MAX (filesystem_management), timer_frequency
 *       (device_management), ui_* (ui), strcmp (lib/string).
 */

#include "modules/cpu_management/commands/getconf/cmd_getconf.h"   // this command
#include "modules/cpu_management/core/affinity/affinity.h"         // affinity_cpu_count
#include "modules/memory_management/core/pmm/pmm.h"                // pmm_get_*_blocks, PAGE_SIZE
#include "modules/process_management/core/process/process.h"       // MAX_PROCESSES, MAX_FDS
#include "modules/filesystem_management/core/vfs/vfs.h"            // FS_NAME_MAX, FS_PATH_MAX
#include "modules/shell/core/line/line.h"                          // LINE_MAX
#include "modules/device_management/timer/timer.h"                 // timer_frequency
#include "modules/ui/core/style/ui.h"                              // ui_*
#include "lib/string/string.h"                                     // strcmp

static uint32_t cpus(void)       { return affinity_cpu_count(); }
static uint32_t page_size(void)  { return PAGE_SIZE; }
static uint32_t phys_pages(void) { return pmm_get_max_blocks(); }
static uint32_t free_pages(void) { return pmm_get_free_blocks(); }
static uint32_t clk_tck(void)    { return timer_frequency(); }
static uint32_t child_max(void)  { return MAX_PROCESSES; }
static uint32_t open_max(void)   { return MAX_FDS; }
static uint32_t line_max(void)   { return LINE_MAX; }
static uint32_t name_max(void)   { return FS_NAME_MAX; }
static uint32_t path_max(void)   { return FS_PATH_MAX; }
static uint32_t long_bit(void)   { return 32; }

static const struct {
    const char *name;
    uint32_t (*value)(void);
    const char *meaning;
} names[] = {
    { "_NPROCESSORS_ONLN", cpus,       "CPUs running now" },
    { "_NPROCESSORS_CONF", cpus,       "CPUs the system has" },
    { "PAGESIZE",          page_size,  "bytes in a memory page" },
    { "PAGE_SIZE",         page_size,  "the same" },
    { "_PHYS_PAGES",       phys_pages, "pages of RAM" },
    { "_AVPHYS_PAGES",     free_pages, "pages of RAM free now" },
    { "CLK_TCK",           clk_tck,    "timer ticks per second" },
    { "CHILD_MAX",         child_max,  "processes at most" },
    { "OPEN_MAX",          open_max,   "open files per process" },
    { "LINE_MAX",          line_max,   "longest shell line" },
    { "NAME_MAX",          name_max,   "longest file name" },
    { "PATH_MAX",          path_max,   "longest path" },
    { "LONG_BIT",          long_bit,   "bits in a long" },
};
#define NAME_COUNT (sizeof(names) / sizeof(names[0]))

void cmd_getconf(const char *args) {
    if (strcmp(args, "-a") == 0) {
        for (unsigned i = 0; i < NAME_COUNT; i++) {
            ui_printf("%-20s %-10u", names[i].name, names[i].value());
            ui_color(UI_DIM);
            ui_printf("%s\n", names[i].meaning);
            ui_reset();
        }
        return;
    }
    if (!args[0]) {
        ui_usage("getconf NAME | getconf -a");
        return;
    }
    for (unsigned i = 0; i < NAME_COUNT; i++) {
        if (strcmp(args, names[i].name) == 0) {
            ui_printf("%u\n", names[i].value());
            return;
        }
    }
    ui_error("getconf", "unknown name (getconf -a lists them)");
}
