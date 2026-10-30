#ifndef CMD_HOST_H
#define CMD_HOST_H

/*
 * cmd_host.h - `host [-t type] name | address`
 *
 * Look a name up in DNS (or an address, backwards).
 */

void cmd_host(const char *args);

#endif
