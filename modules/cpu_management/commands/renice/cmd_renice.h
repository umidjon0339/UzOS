#ifndef CMD_RENICE_H
#define CMD_RENICE_H

/*
 * cmd_renice.h - `renice [-n] N [-p PID... | -u USER...]`
 *
 * Change the niceness of running processes.
 */

void cmd_renice(const char *args);

#endif
