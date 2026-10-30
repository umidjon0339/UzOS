#ifndef DEF_RP_FILTER_H
#define DEF_RP_FILTER_H

/*
 * def_rp_filter.h - DEFENCE rp_filter: a packet whose source would be routed out another interface is dropped
 */

#include <stdint.h>
#include "modules/network_management/core/netdev/netdev.h"     // netdev_t

// Did a packet from 'src' really come in on 'dev'? ipv4_receive() asks
// (not for lo). 1 = plausible, 0 = forged: drop (logged).
int def_rp_ok(uint32_t src, netdev_t *dev);

#endif
