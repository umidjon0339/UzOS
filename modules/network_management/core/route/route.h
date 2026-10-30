#ifndef ROUTE_H
#define ROUTE_H

/*
 * route.h - The routing table: where to send a packet for address X.
 *
 *   destination     gateway     interface
 *   10.0.2.0/24     -           eth0      "on my link: send to X itself"
 *   127.0.0.0/8     -           lo
 *   0.0.0.0/0       10.0.2.2    eth0      "default: everything else, via the router"
 *
 * For a destination, the route with the LONGEST matching prefix wins
 * (/24 beats /0); between equals, the lower metric. The answer is the
 * interface, and the "next hop": the gateway, or X itself when the
 * route has none. ARP then finds the next hop's MAC.
 *
 * Addresses of our own interfaces always go through lo (Linux's
 * "local" table): ping 10.0.2.15 never leaves the machine.
 */

#include <stdint.h>
#include "modules/network_management/core/netdev/netdev.h"     // netdev_t

#define ROUTE_MAX 16

// Who made the route (ip route shows it as "proto ...").
typedef enum {
    ROUTE_KERNEL,           // made by giving an interface an address
    ROUTE_STATIC,           // ip route add / route add
    ROUTE_DHCP,             // the DHCP server said so
} route_proto_t;

typedef struct {
    int used;
    uint32_t dest, mask;    // host order: the network it covers
    uint32_t gateway;       // 0 = directly on the link
    netdev_t *dev;
    uint32_t metric;
    route_proto_t proto;
} route_t;

// The answer of route_lookup().
typedef struct {
    netdev_t *dev;
    uint32_t next_hop;      // whose MAC ARP should find
    uint32_t src;           // our address on that interface
} route_result_t;

// Add / remove a route. 0 on success, else a negative NET_E* error.
int route_add(uint32_t dest, uint32_t mask, uint32_t gateway, netdev_t *dev,
              uint32_t metric, route_proto_t proto);
int route_del(uint32_t dest, uint32_t mask);

// Forget every route through dev (its address went away).
void route_flush_dev(netdev_t *dev);

// Where should a packet for 'dest' go? 1 and *out filled, or 0 (no route).
int route_lookup(uint32_t dest, route_result_t *out);

// Is 'ip' one of our own addresses?
int route_is_local(uint32_t ip);

const route_t *route_get(int slot);         // slot 0 .. ROUTE_MAX-1

#endif
