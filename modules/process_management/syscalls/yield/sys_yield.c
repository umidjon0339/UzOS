/*
 * sys_yield.c - yield(): give the rest of this turn to the next process.
 *
 * Uses: scheduler_yield() (core/scheduler). The caller stays READY and
 *       gets the CPU again on a later turn.
 */

#include "sys_yield.h"
#include "scheduler.h"

void sys_yield(void) {
    scheduler_yield();
}
