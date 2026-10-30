#ifndef DEF_UDP_RATELIMIT_H
#define DEF_UDP_RATELIMIT_H

/*
 * def_udp_ratelimit.h - DEFENCE udp_ratelimit: at most 5 'port unreachable' answers per second to one source
 */

#include <stdint.h>

#define UNREACH_PER_SOURCE 5

// May we answer 'src' (host order) with one more 'port unreachable'?
// udp_receive() asks. 1 = yes, 0 = stay silent (logged once a second).
int def_unreach_ok(uint32_t src);

#endif
