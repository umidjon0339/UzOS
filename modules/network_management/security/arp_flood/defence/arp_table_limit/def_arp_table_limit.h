#ifndef DEF_ARP_TABLE_LIMIT_H
#define DEF_ARP_TABLE_LIMIT_H

/*
 * def_arp_table_limit.h - DEFENCE arp_table_limit: at most 8 ARP entries may come from requests we did not ask for
 */

#include <stdint.h>

#define ARP_UNASKED_MAX 8       // entries learned from requests we did not ask for

// May a request add its sender to the cache, when 'unasked' such
// entries exist already? arp_receive() asks. 1 = yes, 0 = no (logged).
int def_arp_learn_ok(int unasked);

#endif
