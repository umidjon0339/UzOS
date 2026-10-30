#ifndef CMD_DHCLIENT_H
#define CMD_DHCLIENT_H

/*
 * cmd_dhclient.h - `dhclient [-r | -l] [eth0]`
 *
 * Get an address by DHCP (or give it back, or show the lease).
 */

void cmd_dhclient(const char *args);

#endif
