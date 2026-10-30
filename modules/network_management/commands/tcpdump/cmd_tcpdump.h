#ifndef CMD_TCPDUMP_H
#define CMD_TCPDUMP_H

/*
 * cmd_tcpdump.h - `tcpdump [-i dev|any] [-c N] [-n] [-e] [-A] [-X] [-p] [filter]`
 *
 * Watch every packet on the wire, decoded one line each.
 */

void cmd_tcpdump(const char *args);

#endif
