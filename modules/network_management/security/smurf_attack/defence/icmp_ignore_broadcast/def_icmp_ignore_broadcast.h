#ifndef DEF_ICMP_IGNORE_BROADCAST_H
#define DEF_ICMP_IGNORE_BROADCAST_H

/*
 * def_icmp_ignore_broadcast.h - DEFENCE icmp_ignore_broadcast: an echo request sent to a broadcast address is never answered
 */

#include <stdint.h>

// May an echo request whose destination 'was_broadcast' be answered?
// icmp_receive() asks. 1 = yes, 0 = ignore it (logged).
int def_echo_reply_ok(int was_broadcast);

#endif
