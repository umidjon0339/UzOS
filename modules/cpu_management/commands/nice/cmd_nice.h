#ifndef CMD_NICE_H
#define CMD_NICE_H

/*
 * cmd_nice.h - `nice [-n N] [command]`
 *
 * Run a command with a changed niceness, or print the current one.
 */

void cmd_nice(const char *args);

#endif
