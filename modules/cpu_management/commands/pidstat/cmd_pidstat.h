#ifndef CMD_PIDSTAT_H
#define CMD_PIDSTAT_H

/*
 * cmd_pidstat.h - `pidstat [-w] [-p PID] [interval [count]]`
 *
 * CPU use (or context switches) per process.
 */

void cmd_pidstat(const char *args);

#endif
