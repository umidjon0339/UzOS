#ifndef CMD_PROCCALLS_H
#define CMD_PROCCALLS_H

/*
 * cmd_proccalls.h - `proccalls [group]`
 *
 * Call process_management's syscalls from a ring 3 program, check each answer.
 */

void cmd_proccalls(const char *args);

#endif
