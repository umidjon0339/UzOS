/*
 * loopback.c - lo: what goes out comes straight back in.
 *
 * Uses: netdev_register / netdev_receive / netdev_set_address (core/netdev), INET_ADDR
 *       (core/inet), strcpy (lib/string).
 */

#include "modules/network_management/core/loopback/loopback.h"   // this file
#include "modules/network_management/core/inet/inet.h"           // INET_ADDR, inet_mask
#include "lib/string/string.h"                                    // strcpy

static netdev_t lo;

// "Sending" is receiving: netdev queues it in the backlog, so a reply
// to it is handled after this frame, not inside it.
static int lo_transmit(netdev_t *dev, packet_t *p) {
    netdev_receive(dev, p->data, p->len);
    return 1;
}

void loopback_init(void) {
    strcpy(lo.name, "lo");
    lo.flags = NETDEV_UP | NETDEV_LOOPBACK;
    lo.mtu = 1500;                          // limited by our packet buffers (Linux: 65536)
    lo.transmit = lo_transmit;
    netdev_register(&lo);                   // the first one: index 1, like Linux
    netdev_set_address(&lo, INET_ADDR(127, 0, 0, 1), inet_mask(8));
}

netdev_t *loopback_dev(void) {
    return &lo;
}
