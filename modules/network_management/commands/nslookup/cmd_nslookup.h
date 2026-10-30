#ifndef CMD_NSLOOKUP_H
#define CMD_NSLOOKUP_H

/*
 * cmd_nslookup.h - `nslookup name [server]`
 *
 * Ask a DNS server for a name's addresses, showing which server answered.
 */

void cmd_nslookup(const char *args);

#endif
