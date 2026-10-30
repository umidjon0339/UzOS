#ifndef CMD_NETSTAT_H
#define CMD_NETSTAT_H

/*
 * cmd_netstat.h - `netstat [-tuxlanp] | -r | -i | -s`
 *
 * Connections and listening ports, routes, interfaces, counters.
 */

void cmd_netstat(const char *args);

#endif
