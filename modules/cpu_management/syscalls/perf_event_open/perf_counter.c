/*
 * perf_counter.c - Counters behind file descriptors.
 *
 * Each open counter remembers its value at the moment it was opened;
 * a read returns "now - then". When the last descriptor to it is closed
 * (open_count 0), its slot can be used again.
 *
 * Uses: cpu_account_cycles / cpu_account_switches (cpu_management
 *       core/accounting), tsc_now / tsc_mhz (core/tsc), process_get_by_pid
 *       (process_management), memcpy / memset / strncpy (lib/string).
 */

#include "modules/cpu_management/syscalls/perf_event_open/perf_counter.h"   // this file
#include "modules/cpu_management/core/accounting/cpu_account.h"            // cpu_account_cycles, _switches
#include "modules/cpu_management/core/tsc/tsc.h"                           // tsc_now, tsc_mhz
#include "lib/string/string.h"                                             // memcpy, memset, strncpy

typedef struct {
    vfs_node_t node;            // FIRST: a pointer to the node is a pointer to the counter
    int        used;
    uint32_t   pid;             // 0 = system-wide
    uint32_t   type;
    uint64_t   config;
    uint64_t   start;           // the value when it was opened
    uint64_t   last;            // the last value read (kept after the process ends)
} perf_counter_t;

static perf_counter_t counters[PERF_MAX_EVENTS];

int perf_counter_supported(uint32_t type, uint64_t config) {
    if (type == PERF_TYPE_HARDWARE)
        return config == PERF_COUNT_HW_CPU_CYCLES;
    if (type == PERF_TYPE_SOFTWARE)
        return config <= PERF_COUNT_SW_CPU_MIGRATIONS;
    return 0;
}

// The count of an event for 'p' right now, from its start. p = NULL:
// the whole system since boot.
static uint64_t value_of(const process_t *p, uint32_t type, uint64_t config) {
    uint64_t cycles = p ? cpu_account_cycles(p) : tsc_now();

    if (type == PERF_TYPE_HARDWARE)
        return cycles;
    switch (config) {
        case PERF_COUNT_SW_CPU_CLOCK:
        case PERF_COUNT_SW_TASK_CLOCK:
            return tsc_mhz() ? cycles * 1000 / tsc_mhz() : 0;      // nanoseconds
        case PERF_COUNT_SW_CONTEXT_SWITCHES:
            return p ? p->nvcsw + p->nivcsw : cpu_account_switches();
        default:
            return 0;               // page faults, migrations: none happen
    }
}

// read() on a counter: 8 bytes, the count since it was opened.
static uint32_t counter_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buf) {
    perf_counter_t *c = (perf_counter_t *)node;
    (void)offset;                   // a counter has no position: every read is "now"
    if (size < sizeof(uint64_t))
        return 0;

    process_t *p = c->pid ? process_get_by_pid(c->pid) : 0;
    if (!c->pid || p)
        c->last = value_of(p, c->type, c->config) - c->start;
    memcpy(buf, &c->last, sizeof(uint64_t));
    return sizeof(uint64_t);
}

vfs_node_t *perf_counter_create(uint32_t pid, uint32_t type, uint64_t config) {
    if (!perf_counter_supported(type, config))
        return 0;
    process_t *p = pid ? process_get_by_pid(pid) : 0;
    if (pid && !p)
        return 0;

    for (int i = 0; i < PERF_MAX_EVENTS; i++) {
        perf_counter_t *c = &counters[i];
        if (c->used && c->node.open_count > 0)
            continue;                           // still open somewhere
        memset(c, 0, sizeof(*c));
        c->used   = 1;
        c->pid    = pid;
        c->type   = type;
        c->config = config;
        c->start  = value_of(p, type, config);
        strncpy(c->node.name, "[perf_event]", FS_NAME_MAX);
        c->node.flags  = FS_FILE;
        c->node.mode   = 044;                   // readable (user_management/core/perm)
        c->node.owner  = p ? p->uid : 0;
        c->node.read   = counter_read;
        c->node.parent = &c->node;              // in no directory
        return &c->node;
    }
    return 0;
}
