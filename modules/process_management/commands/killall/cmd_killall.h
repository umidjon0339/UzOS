#ifndef CMD_KILLALL_H
#define CMD_KILLALL_H

/*
 * cmd_killall.h - `killall [-SIGNAL] name`
 *
 * Signal every process with exactly this name.
 */

void cmd_killall(const char *args);

#endif
