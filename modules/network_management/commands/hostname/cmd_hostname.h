#ifndef CMD_HOSTNAME_H
#define CMD_HOSTNAME_H

/*
 * cmd_hostname.h - `hostname [-I | -d | name]`
 *
 * Show or set this machine's name; -I shows its addresses.
 */

void cmd_hostname(const char *args);

#endif
