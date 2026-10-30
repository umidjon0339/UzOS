#ifndef CMD_FSCALLS_H
#define CMD_FSCALLS_H

/*
 * cmd_fscalls.h - `fscalls [group]`
 *
 * Call filesystem_management's syscalls from a ring 3 program, check each answer.
 */

void cmd_fscalls(const char *args);

#endif
