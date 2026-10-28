#ifndef PERF_COUNTER_H
#define PERF_COUNTER_H

/*
 * perf_counter.h - The counters perf_event_open() hands out.
 *
 * A counter is a VFS node whose read() computes the value on the spot
 * (like a file in /proc). So the ordinary descriptor machinery of
 * io_management works for it: read() reads it, close() and the
 * fd_cleanup defence release it.
 *
 * Real CPUs have a PMU (performance monitoring unit) that counts
 * instructions, cache misses, ... in hardware. QEMU's CPU has none that
 * UzOS drives, so UzOS offers what the kernel itself can measure:
 *
 *   type      config              counted by
 *   HARDWARE  CPU_CYCLES          the TSC while the process ran (exact)
 *   HARDWARE  INSTRUCTIONS        - not supported (needs a PMU) -
 *   SOFTWARE  CPU_CLOCK, TASK_CLOCK  nanoseconds it ran (TSC based)
 *   SOFTWARE  PAGE_FAULTS         0: UzOS never pages in lazily
 *   SOFTWARE  CONTEXT_SWITCHES    task switches away from it
 *   SOFTWARE  CPU_MIGRATIONS      0: one CPU, nowhere to migrate to
 */

#include <stdint.h>
#include "modules/filesystem_management/core/vfs/vfs.h"         // vfs_node_t

#define PERF_TYPE_HARDWARE 0
#define PERF_TYPE_SOFTWARE 1

#define PERF_COUNT_HW_CPU_CYCLES   0
#define PERF_COUNT_HW_INSTRUCTIONS 1

#define PERF_COUNT_SW_CPU_CLOCK        0
#define PERF_COUNT_SW_TASK_CLOCK       1
#define PERF_COUNT_SW_PAGE_FAULTS      2
#define PERF_COUNT_SW_CONTEXT_SWITCHES 3
#define PERF_COUNT_SW_CPU_MIGRATIONS   4

#define PERF_MAX_EVENTS 16      // open counters at the same time

// 1 if UzOS can count this event.
int perf_counter_supported(uint32_t type, uint64_t config);

// Open a counter for 'pid' (0 = the whole system). Returns the node a
// descriptor should point to; reading it gives the count since now.
// NULL if not supported, no such process, or all counters are in use.
vfs_node_t *perf_counter_create(uint32_t pid, uint32_t type, uint64_t config);

#endif
