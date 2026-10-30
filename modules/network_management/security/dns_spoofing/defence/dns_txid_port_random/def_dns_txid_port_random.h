#ifndef DEF_DNS_TXID_PORT_RANDOM_H
#define DEF_DNS_TXID_PORT_RANDOM_H

/*
 * def_dns_txid_port_random.h - DEFENCE dns_txid_port_random: every DNS query uses a random id and a random source port
 */

#include <stdint.h>

#define DNS_PORT_LOW  49152     // random source ports come from here ...
#define DNS_PORT_HIGH 65535     // ... to here

// A fresh random 16-bit query id, and a random source port.
uint16_t def_dns_id(void);
uint16_t def_dns_port(void);

#endif
