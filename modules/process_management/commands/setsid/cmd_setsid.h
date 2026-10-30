#ifndef CMD_SETSID_H
#define CMD_SETSID_H

/*
 * cmd_setsid.h - `setsid command [args]`
 *
 * Run a command in the background, in a new session of its own.
 */

void cmd_setsid(const char *args);

#endif
