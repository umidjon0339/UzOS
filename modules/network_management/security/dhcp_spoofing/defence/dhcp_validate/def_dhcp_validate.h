#ifndef DEF_DHCP_VALIDATE_H
#define DEF_DHCP_VALIDATE_H

/*
 * def_dhcp_validate.h - DEFENCE dhcp_validate: an offer must be self-consistent, and once we had a server, from that server
 */

#include <stdint.h>

// Is an OFFER of 'ip'/'mask' with 'router' from 'server' acceptable,
// when 'known_server' gave us our last lease (0 = none yet)? dhcp asks.
// 1 = yes, 0 = refuse (logged).
int def_dhcp_offer_ok(uint32_t ip, uint32_t mask, uint32_t router, uint32_t server, uint32_t known_server);

#endif
