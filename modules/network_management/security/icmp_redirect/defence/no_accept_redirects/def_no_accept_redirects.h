#ifndef DEF_NO_ACCEPT_REDIRECTS_H
#define DEF_NO_ACCEPT_REDIRECTS_H

/*
 * def_no_accept_redirects.h - DEFENCE no_accept_redirects: ICMP redirects never change the routing table
 */

#include <stdint.h>

// May a redirect change the route to 'host'? icmp_receive() asks.
// Always 0 in UzOS (logged).
int def_redirect_ok(uint32_t host);

#endif
