#ifndef DEF_ICMP_RATELIMIT_H
#define DEF_ICMP_RATELIMIT_H

/*
 * def_icmp_ratelimit.h - DEFENCE icmp_ratelimit: at most 10 ICMP replies and errors per 100 ms
 */

#include <stdint.h>

#define ICMP_BURST 10           // ICMP messages we send per NET_TICK_MS

// May we send one more ICMP message now? icmp asks before each reply or
// error. 1 = yes, 0 = no (counted; logged once per tick).
int def_icmp_send_ok(void);

#endif
