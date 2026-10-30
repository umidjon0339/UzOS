#ifndef USERTASK_H
#define USERTASK_H

/*
 * usertask.h - Run a small piece of machine code as a ring 3 process.
 *
 * `testuser` and several security lab attacks need "a user program that
 * does exactly this one thing". They write it in assembly (a .s file in
 * their own folder) and hand its start and end to usertask_start():
 *
 *     extern uint8_t my_program[], my_program_end[];
 *     int pid = usertask_start("myprog", my_program, my_program_end, 1);
 *     usertask_wait(pid, 500);              // until it ends or is killed
 *
 * Every program is copied to the same two user pages:
 *
 *     USERTASK_CODE     its code (and the mailbox at the end of the page)
 *     USERTASK_STACK    its stack
 *
 * so only one user task runs at a time. The MAILBOX is a small area a
 * program may write its results to; the kernel reads it after the
 * program ended (it is cleared before each start).
 */

#include <stdint.h>
#include "modules/memory_management/core/layout/memory_layout.h"  // USER_TEST_CODE, PAGE_SIZE

#define USERTASK_CODE          USER_TEST_CODE               // see memory_layout.h
#define USERTASK_STACK         (USER_TEST_CODE + PAGE_SIZE)
#define USERTASK_MAILBOX_SIZE  256
#define USERTASK_MAILBOX       (USERTASK_CODE + PAGE_SIZE - USERTASK_MAILBOX_SIZE)
#define USERTASK_MAX_CODE      (PAGE_SIZE - USERTASK_MAILBOX_SIZE)

// Start the code between 'start' and 'end' in ring 3, as user 'uid'.
// Returns the PID, or -1 (too big, out of memory, no free slot).
int usertask_start(const char *name, const uint8_t *start, const uint8_t *end,
                   uint32_t uid);

// Sleep until process 'pid' has ended, at most 'timeout_ms'.
// 1 = it ended (exit or killed), 0 = it is still running.
int usertask_wait(int pid, uint32_t timeout_ms);

// The mailbox, as the kernel sees it: 64 32-bit words.
volatile uint32_t *usertask_mailbox(void);

#endif
