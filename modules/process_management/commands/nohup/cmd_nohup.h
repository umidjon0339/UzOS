#ifndef CMD_NOHUP_H
#define CMD_NOHUP_H

/*
 * cmd_nohup.h - `nohup command [args]`
 *
 * Run a command in the background, immune to the hangup at logout.
 */

void cmd_nohup(const char *args);

#endif
