#ifndef CMD_ARP_H
#define CMD_ARP_H

/*
 * cmd_arp.h - `arp [-a] | arp -s IP MAC [dev] | arp -d IP`
 *
 * The ARP cache in the classic net-tools look (ip neigh is the new one).
 */

void cmd_arp(const char *args);

#endif
