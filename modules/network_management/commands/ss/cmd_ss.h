#ifndef CMD_SS_H
#define CMD_SS_H

/*
 * cmd_ss.h - `ss [-tuxlap] | ss -s`
 *
 * Socket statistics (iproute2): the newer netstat.
 */

void cmd_ss(const char *args);

#endif
