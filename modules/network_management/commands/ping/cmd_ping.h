#ifndef CMD_PING_H
#define CMD_PING_H

/*
 * cmd_ping.h - `ping [-c count] [-s size] [-t ttl] [-i seconds] host`
 *
 * Send ICMP echo requests and time the replies.
 */

void cmd_ping(const char *args);

#endif
