#ifndef LOOPBACK_H
#define LOOPBACK_H

/*
 * loopback.h - "lo": the interface that talks to itself (127.0.0.1).
 *
 * Every frame sent on lo is received on lo at once. Programs on the
 * same machine talk through it (ping 127.0.0.1, a server and its client),
 * and it works without any network card.
 */

#include "modules/network_management/core/netdev/netdev.h"     // netdev_t

// Register lo with address 127.0.0.1/8. Called once at boot.
void loopback_init(void);

netdev_t *loopback_dev(void);

#endif
