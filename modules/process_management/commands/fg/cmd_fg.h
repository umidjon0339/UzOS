#ifndef CMD_FG_H
#define CMD_FG_H

/*
 * cmd_fg.h - `fg [%job]`
 *
 * Bring a job to the foreground (continue it if stopped) and wait for it.
 */

void cmd_fg(const char *args);

#endif
