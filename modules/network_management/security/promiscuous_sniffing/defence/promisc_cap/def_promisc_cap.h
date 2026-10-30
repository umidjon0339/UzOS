#ifndef DEF_PROMISC_CAP_H
#define DEF_PROMISC_CAP_H

/*
 * def_promisc_cap.h - DEFENCE promisc_cap: only CAP_NET_RAW may switch an interface to promiscuous mode
 */

#include <stdint.h>

// May user 'uid' change promiscuous mode? netdev_set_promisc() asks.
// 1 = yes, 0 = refused (logged).
int def_promisc_ok(uint32_t uid);

#endif
