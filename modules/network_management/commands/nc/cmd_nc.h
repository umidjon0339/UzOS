#ifndef CMD_NC_H
#define CMD_NC_H

/*
 * cmd_nc.h - `nc [-u] host port | nc -l [-u] port | nc -z host port[-port]`
 *
 * netcat: read and write raw TCP or UDP, as a client or a server.
 */

void cmd_nc(const char *args);

#endif
