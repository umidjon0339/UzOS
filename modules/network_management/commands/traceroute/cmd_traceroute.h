#ifndef CMD_TRACEROUTE_H
#define CMD_TRACEROUTE_H

/*
 * cmd_traceroute.h - `traceroute [-m max_hops] [-q probes] host`
 *
 * The routers on the way to a host, one TTL at a time.
 */

void cmd_traceroute(const char *args);

#endif
