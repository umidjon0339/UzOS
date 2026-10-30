#ifndef DEF_RANDOM_IP_ID_H
#define DEF_RANDOM_IP_ID_H

/*
 * def_random_ip_id.h - DEFENCE random_ip_id: every IP id is unpredictable
 */

#include <stdint.h>

// The identification field of the next packet we send. ipv4_send() asks.
uint16_t def_next_ip_id(void);

#endif
