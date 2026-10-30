#ifndef CMD_TIMEOUT_H
#define CMD_TIMEOUT_H

/*
 * cmd_timeout.h - `timeout [-s SIG] seconds command [args]`
 *
 * Run a command; end it if it takes too long.
 */

void cmd_timeout(const char *args);

#endif
