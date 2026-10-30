#ifndef DEF_REASSEMBLE_BEFORE_FILTER_H
#define DEF_REASSEMBLE_BEFORE_FILTER_H

/*
 * def_reassemble_before_filter.h - DEFENCE reassemble_before_filter: the firewall sees only whole datagrams: fragments are reassembled first
 */

#include <stdint.h>

// May a packet with this 'frag' field (host order) go to the firewall?
// ipv4_receive() asks. 1 = a whole datagram, 0 = a fragment: never (logged).
int def_filter_whole(uint16_t frag);

#endif
