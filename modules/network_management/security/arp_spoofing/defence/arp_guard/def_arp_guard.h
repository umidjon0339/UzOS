#ifndef DEF_ARP_GUARD_H
#define DEF_ARP_GUARD_H

/*
 * def_arp_guard.h - DEFENCE arp_guard: an ARP packet may not change a known MAC, nor add one nobody asked for
 */

#include <stdint.h>

// May an ARP packet set the MAC of its sender? 'state' = the sender's
// entry now (ARP_FREE if none), 'same_mac' = its MAC equals the one we
// have, 'request_for_us' = it is a request asking for OUR address.
// arp_receive() asks this. 1 = store it, 0 = ignore it (logged).
int def_arp_guard_ok(int state, int same_mac, int request_for_us);

#endif
