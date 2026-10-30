#ifndef CMD_IP_H
#define CMD_IP_H

/*
 * cmd_ip.h - `ip [-s] link | addr | route | neigh ...`
 *
 * Show and change the network: interfaces, addresses, routes, neighbours.
 * Each object has its own file in this folder (ip_link.c, ...).
 */

void cmd_ip(const char *args);

struct netdev;

// One per object; 'rest' is what follows the object's name.
void ip_link(const char *rest, int stats);
void ip_addr(const char *rest);
void ip_neigh(const char *rest);
void ip_route(const char *rest);

// The two "link" lines of one interface (ip link and ip addr start so).
void ip_link_print(struct netdev *dev, int stats);

#endif
