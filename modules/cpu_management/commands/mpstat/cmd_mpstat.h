#ifndef CMD_MPSTAT_H
#define CMD_MPSTAT_H

/*
 * cmd_mpstat.h - `mpstat [-P ALL|0] [interval [count]]`
 *
 * CPU utilization per CPU.
 */

void cmd_mpstat(const char *args);

#endif
