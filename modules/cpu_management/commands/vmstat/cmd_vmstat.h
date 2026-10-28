#ifndef CMD_VMSTAT_H
#define CMD_VMSTAT_H

/*
 * cmd_vmstat.h - `vmstat [interval [count]]`
 *
 * Processes, memory, interrupts, context switches and CPU, per interval.
 */

void cmd_vmstat(const char *args);

#endif
