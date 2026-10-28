#ifndef CMD_TASKSET_H
#define CMD_TASKSET_H

/*
 * cmd_taskset.h - `taskset [-c] [-p] [mask|list] [PID | command]`
 *
 * Show or set the CPU affinity of a process.
 */

void cmd_taskset(const char *args);

#endif
