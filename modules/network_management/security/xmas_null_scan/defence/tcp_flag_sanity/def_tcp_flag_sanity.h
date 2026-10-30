#ifndef DEF_TCP_FLAG_SANITY_H
#define DEF_TCP_FLAG_SANITY_H

/*
 * def_tcp_flag_sanity.h - DEFENCE tcp_flag_sanity: segments with impossible flag combinations are dropped silently
 */

#include <stdint.h>

// Are these TCP flags a real segment? tcp_receive() asks.
// 1 = yes, 0 = a scan: drop (logged).
int def_tcp_flags_ok(uint8_t flags);

#endif
