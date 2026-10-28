#ifndef CMD_TURBOSTAT_H
#define CMD_TURBOSTAT_H

/*
 * cmd_turbostat.h - `turbostat [-i SECONDS] [-n COUNT]`
 *
 * Frequency, busy time, interrupts and idle states per interval.
 */

void cmd_turbostat(const char *args);

#endif
