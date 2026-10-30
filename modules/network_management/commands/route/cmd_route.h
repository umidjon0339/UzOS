#ifndef CMD_ROUTE_H
#define CMD_ROUTE_H

/*
 * cmd_route.h - `route [-n] | route add|del ...`
 *
 * The routing table in the classic net-tools look (ip route is the new one).
 */

void cmd_route(const char *args);

#endif
