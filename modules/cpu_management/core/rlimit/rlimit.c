/*
 * rlimit.c - The limit table, its rules, and the checks the rest of the
 *            kernel calls.
 *
 * Uses: caps_has (security_management) for CAP_RESOURCE, the process
 *       table (process_management), timer_frequency (device_management),
 *       ui_* (ui) to report a process killed by RLIMIT_CPU.
 */

#include "modules/cpu_management/core/rlimit/rlimit.h"              // this part
#include "modules/security_management/core/caps/caps.h"             // caps_has, CAP_RESOURCE
#include "modules/device_management/timer/timer.h"                  // timer_frequency
#include "modules/ui/core/style/ui.h"                               // ui_color, ui_printf

#define STACK_BYTES 4096    // PROCESS_STACK_SIZE: one page, for every process

// What UzOS does with each limit. A row with name 0 = not used.
static const struct {
    const char *name;
    const char *unit;
    char option;            // the `ulimit` letter
    const char *enforced_in;
    uint32_t soft, hard;    // the defaults of a process with no parent
} table[RLIMIT_COUNT] = {
    [RLIMIT_CPU]    = { "cpu time",     "seconds", 't', "timer tick (core/accounting)", RLIM_INFINITY, RLIM_INFINITY },
    [RLIMIT_FSIZE]  = { "file size",    "bytes",   'f', "write() (io_management)",      RLIM_INFINITY, RLIM_INFINITY },
    [RLIMIT_DATA]   = { "data seg size","bytes",   'd', "sbrk() (memory_management)",   RLIM_INFINITY, RLIM_INFINITY },
    [RLIMIT_STACK]  = { "stack size",   "bytes",   's', "fixed: one 4 KiB page",        STACK_BYTES,   STACK_BYTES   },
    [RLIMIT_NPROC]  = { "max user processes", "processes", 'u', "process_create()",     MAX_PROCESSES, MAX_PROCESSES },
    [RLIMIT_NOFILE] = { "open files",   "files",   'n', "open(), perf_event_open()",    MAX_FDS,       MAX_FDS       },
    [RLIMIT_NICE]   = { "scheduling priority", "20 - nice", 'e', "setpriority() (priority_bounds)", 0, 0 },
    [RLIMIT_RTPRIO] = { "real-time priority",  "1..99",     'r', "sched_setscheduler() (priority_bounds)", 0, 0 },
};

int rlimit_supported(int r) {
    return r >= 0 && r < RLIMIT_COUNT && table[r].name != 0;
}

const char *rlimit_name(int r)       { return rlimit_supported(r) ? table[r].name : 0; }
const char *rlimit_unit(int r)       { return rlimit_supported(r) ? table[r].unit : 0; }
char        rlimit_option(int r)     { return rlimit_supported(r) ? table[r].option : 0; }
const char *rlimit_enforced_in(int r){ return rlimit_supported(r) ? table[r].enforced_in : 0; }

void rlimit_defaults(process_t *p) {
    for (int r = 0; r < RLIMIT_COUNT; r++) {
        p->rlim_cur[r] = rlimit_supported(r) ? table[r].soft : RLIM_INFINITY;
        p->rlim_max[r] = rlimit_supported(r) ? table[r].hard : RLIM_INFINITY;
    }
}

int rlimit_get(const process_t *p, int r, rlimit_t *out) {
    if (!p || r < 0 || r >= RLIMIT_COUNT)
        return 0;
    out->cur = p->rlim_cur[r];
    out->max = p->rlim_max[r];
    return 1;
}

int rlimit_set(const process_t *caller, process_t *target, int r, const rlimit_t *limit) {
    uint32_t uid = caller ? caller->uid : 0;
    int privileged = caps_has(uid, CAP_RESOURCE);

    if (!target || !rlimit_supported(r) || limit->cur > limit->max)
        return 0;
    if (caller && caller->uid != target->uid && !privileged)
        return 0;                               // not your process
    if (limit->max > target->rlim_max[r] && !privileged)
        return 0;                               // raising the ceiling needs root
    if (r == RLIMIT_STACK && limit->cur != STACK_BYTES)
        return 0;                               // stacks have one size
    if (r == RLIMIT_NOFILE && limit->max > MAX_FDS && limit->max != RLIM_INFINITY)
        return 0;                               // the fd table has 16 slots
    if (r == RLIMIT_NICE && limit->max != RLIM_INFINITY && limit->max > 40)
        return 0;                               // 20 - (-20) = 40 is the most
    if (r == RLIMIT_RTPRIO && limit->max != RLIM_INFINITY && limit->max > 99)
        return 0;

    target->rlim_cur[r] = limit->cur;
    target->rlim_max[r] = limit->max;
    return 1;
}

/* ---------- Enforcement ---------- */

void rlimit_check_cpu(process_t *p) {
    uint32_t limit = p->rlim_cur[RLIMIT_CPU];
    uint32_t hz = timer_frequency();

    if (p->is_system || limit == RLIM_INFINITY || !hz)
        return;
    if ((p->user_ticks + p->kernel_ticks) / hz < limit)
        return;

    // Linux sends SIGXCPU here; UzOS has no signal handlers, so the
    // default action happens: the process ends. The scheduler switches
    // away on this same tick and the reaper frees it later.
    p->state = PROCESS_STATE_ZOMBIE;

    uint8_t color = vga_get_color();
    ui_color(UI_WARNING);
    ui_printf("\n[rlimit] PID %u (%s) used its %u s of CPU: killed (RLIMIT_CPU)\n",
              p->pid, p->name, limit);
    vga_set_color_byte(color);
}

int rlimit_nproc_ok(const process_t *creator, uint32_t uid) {
    if (!creator || caps_has(creator->uid, CAP_RESOURCE))
        return 1;                               // the kernel, or root

    process_t *t = process_get_table();
    uint32_t count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (t[i].uid == uid && t[i].state != PROCESS_STATE_FREE &&
            t[i].state != PROCESS_STATE_ZOMBIE)
            count++;
    return count < creator->rlim_cur[RLIMIT_NPROC];
}

int rlimit_nofile(const process_t *p) {
    uint32_t limit = p ? p->rlim_cur[RLIMIT_NOFILE] : MAX_FDS;
    return limit > MAX_FDS ? MAX_FDS : (int)limit;
}

int rlimit_fsize_ok(const process_t *p, uint32_t end) {
    return !p || p->rlim_cur[RLIMIT_FSIZE] == RLIM_INFINITY || end <= p->rlim_cur[RLIMIT_FSIZE];
}

int rlimit_data_ok(const process_t *p, uint32_t bytes) {
    return !p || p->rlim_cur[RLIMIT_DATA] == RLIM_INFINITY || bytes <= p->rlim_cur[RLIMIT_DATA];
}
