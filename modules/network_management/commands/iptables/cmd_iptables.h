#ifndef CMD_IPTABLES_H
#define CMD_IPTABLES_H

/*
 * cmd_iptables.h - `iptables -L | -A/-I CHAIN rule | -D CHAIN n | -F | -P CHAIN TARGET | -Z`
 *
 * Show and change the packet filter (core/firewall).
 */

void cmd_iptables(const char *args);

#endif
