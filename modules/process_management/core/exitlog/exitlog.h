#ifndef EXITLOG_H
#define EXITLOG_H

/*
 * exitlog.h - What became of finished processes, until their parent asks.
 *
 * UzOS frees a zombie on the next timer tick (the reaper), so a parent
 * that calls wait4() a second later would find nothing. Instead, the
 * reaper writes one record per finished process here, and wait4 /
 * waitpid / waitid TAKE it (each record is reported once):
 *
 *   child exits -> ZOMBIE -> reaper: exitlog_add() + free the slot
 *   parent: wait4(-1, &status, 0, &usage) -> exitlog_take() -> child's pid
 *
 * Kept: pid, parent, group, owner, exit status (the same encoding as
 * Linux: exit(n) = n << 8, signal s = s, | 0x80 = core dumped) and the
 * CPU it used. When the log is full the oldest record goes.
 */

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

#define EXITLOG_SIZE 32

typedef struct {
    uint32_t pid, ppid, pgid, uid;
    int      status;
    uint32_t user_ticks, kernel_ticks;      // its own and its children's
    uint32_t nvcsw, nivcsw;
} exit_record_t;

// Called by the reaper right before it frees 'p'.
void exitlog_add(const process_t *p);

// Which finished children a wait asks for (as wait4's 'pid' argument):
//   pid > 0   that child       pid == -1   any child
//   pid == 0  any child in group 'my_pgid'   pid < -1  any in group -pid
// Take (remove) the oldest matching record of a child of 'parent'.
// 1 = found, copied to *out (if not NULL).
int exitlog_take(uint32_t parent, int32_t pid, uint32_t my_pgid, exit_record_t *out);

// Is there a record for 'pid' at all (whoever its parent was)?
int exitlog_find(uint32_t pid, exit_record_t *out);

#endif
