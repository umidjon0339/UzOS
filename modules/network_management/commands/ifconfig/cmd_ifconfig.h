#ifndef CMD_IFCONFIG_H
#define CMD_IFCONFIG_H

/*
 * cmd_ifconfig.h - `ifconfig [dev [address] [netmask M] [up|down] [mtu N] [promisc|-promisc]]`
 *
 * Show or set up the interfaces, net-tools style (ip addr / ip link is the new way).
 */

void cmd_ifconfig(const char *args);

#endif
