#ifndef DEF_MAC_LOCK_H
#define DEF_MAC_LOCK_H

/*
 * def_mac_lock.h - DEFENCE mac_lock: only CAP_NET_ADMIN may change an interface's MAC address
 */

#include <stdint.h>

// May user 'uid' change an interface's MAC? netdev_set_mac() asks.
// 1 = yes, 0 = refused (logged).
int def_mac_change_ok(uint32_t uid);

#endif
