#ifndef CMD_NMAP_H
#define CMD_NMAP_H

/*
 * cmd_nmap.h - `nmap [-sT|-sS|-sU|-sn] [-p ports] host|net/24`
 *
 * Find which ports of a host are open (or which hosts are up).
 */

void cmd_nmap(const char *args);

#endif
