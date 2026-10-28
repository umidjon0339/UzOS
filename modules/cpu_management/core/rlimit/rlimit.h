#ifndef RLIMIT_H
#define RLIMIT_H

/*
 * rlimit.h - Resource limits: how much of something one process may use.
 *
 * Every process carries a table of limits, numbered as in Linux. Each
 * limit has two values:
 *
 *   soft (cur)   the limit that is enforced right now
 *   hard (max)   the ceiling for the soft one
 *
 * Anybody may lower either value, or move the soft one up to the hard
 * one. Raising the HARD limit needs CAP_RESOURCE (root). A new process
 * inherits its parent's limits (core/task), so `ulimit -t 2` in the
 * shell limits every command started after it.
 *
 *   #   limit     unit       UzOS enforces it in
 *   0   CPU       seconds    the timer tick: over the limit = killed
 *   1   FSIZE     bytes      write(): a file may not grow past it
 *   2   DATA      bytes      sbrk(): the heap may not grow past it
 *   3   STACK     bytes      fixed: every stack is one 4 KiB page
 *   6   NPROC     processes  process_create(): per user (root is exempt)
 *   7   NOFILE    files      open(), perf_event_open(): fds 0 .. limit-1 only
 *   13  NICE      20 - nice  setpriority(): how far a user may lower nice
 *   14  RTPRIO    1..99      sched_setscheduler(): real-time without root
 *
 * The other Linux limits (CORE, RSS, MEMLOCK, AS, ...) have nothing to
 * limit in UzOS. They read as "unlimited" and cannot be set.
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

#define RLIM_INFINITY 0xFFFFFFFFu   // "unlimited"

#define RLIMIT_CPU      0
#define RLIMIT_FSIZE    1
#define RLIMIT_DATA     2
#define RLIMIT_STACK    3
#define RLIMIT_CORE     4
#define RLIMIT_RSS      5
#define RLIMIT_NPROC    6
#define RLIMIT_NOFILE   7
#define RLIMIT_MEMLOCK  8
#define RLIMIT_AS       9
#define RLIMIT_LOCKS    10
#define RLIMIT_SIGPENDING 11
#define RLIMIT_MSGQUEUE 12
#define RLIMIT_NICE     13
#define RLIMIT_RTPRIO   14
#define RLIMIT_RTTIME   15
#define RLIMIT_COUNT    PROCESS_RLIMITS     // 16

// One limit, as getrlimit()/setrlimit() pass it (Linux struct rlimit).
typedef struct {
    uint32_t cur;       // soft
    uint32_t max;       // hard
} rlimit_t;

// The same with 64-bit values, for prlimit64() (Linux struct rlimit64).
typedef struct {
    uint64_t cur;
    uint64_t max;
} rlimit64_t;
#define RLIM64_INFINITY 0xFFFFFFFFFFFFFFFFull

// Name ("cpu"), unit ("seconds"), ulimit option letter ('t') and where
// UzOS enforces it; NULL / 0 for a limit UzOS does not use.
const char *rlimit_name(int resource);
const char *rlimit_unit(int resource);
char        rlimit_option(int resource);
const char *rlimit_enforced_in(int resource);

// 1 if UzOS uses limit 'resource' (the table above).
int rlimit_supported(int resource);

// Give a process with no parent (idle, kernel_main) the default limits.
void rlimit_defaults(process_t *p);

// Read one limit of 'p'. 1 = done, 0 = no such limit.
int rlimit_get(const process_t *p, int resource, rlimit_t *out);

// Set one limit of 'target', asked by 'caller' (NULL = the kernel).
// Refused (0) if: unknown/unused limit, soft > hard, raising the hard
// limit without CAP_RESOURCE, someone else's process without
// CAP_RESOURCE, or a value UzOS cannot give (STACK, NOFILE > 16).
int rlimit_set(const process_t *caller, process_t *target, int resource,
               const rlimit_t *limit);

/* ---------- Enforcement: called where each resource is used ---------- */

// The timer tick (core/accounting): kill 'p' if it used more CPU
// seconds than RLIMIT_CPU allows. System processes are never killed.
void rlimit_check_cpu(process_t *p);

// process_create (process_management): may 'creator' start another
// process for user 'uid'? Holders of CAP_RESOURCE always may.
int rlimit_nproc_ok(const process_t *creator, uint32_t uid);

// open (io_management), perf_event_open: the first descriptor number
// 'p' may NOT use.
int rlimit_nofile(const process_t *p);

// write (io_management): may 'p' make a file 'end' bytes long?
int rlimit_fsize_ok(const process_t *p, uint32_t end);

// sbrk (memory_management): may the heap of 'p' be 'bytes' long?
int rlimit_data_ok(const process_t *p, uint32_t bytes);

#endif
