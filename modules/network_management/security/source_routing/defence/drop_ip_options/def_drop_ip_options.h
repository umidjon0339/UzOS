#ifndef DEF_DROP_IP_OPTIONS_H
#define DEF_DROP_IP_OPTIONS_H

/*
 * def_drop_ip_options.h - DEFENCE drop_ip_options: packets with source route options (LSRR, SSRR) are dropped
 */

#include <stdint.h>

// Are the IP options at 'opt' (len bytes) acceptable? ipv4_receive()
// asks. 1 = yes, 0 = a source route: drop (logged).
int def_ip_options_ok(const uint8_t *opt, uint32_t len);

#endif
