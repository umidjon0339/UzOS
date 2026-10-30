#ifndef CMD_WAIT_H
#define CMD_WAIT_H

/*
 * cmd_wait.h - `wait [%job | pid]...`
 *
 * Wait until jobs end and report how each ended.
 */

void cmd_wait(const char *args);

#endif
