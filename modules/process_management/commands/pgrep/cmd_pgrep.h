#ifndef CMD_PGREP_H
#define CMD_PGREP_H

/*
 * cmd_pgrep.h - `pgrep [-l] [-x] [-u user] pattern`
 *
 * Find processes by name; print their PIDs.
 */

void cmd_pgrep(const char *args);

#endif
