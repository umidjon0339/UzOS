#ifndef CMD_IFUP_H
#define CMD_IFUP_H

/*
 * cmd_ifup.h - `ifup [eth0]`
 *
 * Bring an interface up and get it an address by DHCP.
 */

void cmd_ifup(const char *args);

#endif
