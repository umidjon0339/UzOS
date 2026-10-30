#ifndef CMD_NETCALLS_H
#define CMD_NETCALLS_H

/*
 * cmd_netcalls.h - `netcalls [udp | tcp | local]`
 *
 * Ring 3 programs call the network syscalls; the answers are checked.
 */

void cmd_netcalls(const char *args);

#endif
