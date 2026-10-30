#ifndef CMD_PKILL_H
#define CMD_PKILL_H

/*
 * cmd_pkill.h - `pkill [-SIGNAL] [-x] [-u user] pattern`
 *
 * Signal every process whose name matches.
 */

void cmd_pkill(const char *args);

#endif
