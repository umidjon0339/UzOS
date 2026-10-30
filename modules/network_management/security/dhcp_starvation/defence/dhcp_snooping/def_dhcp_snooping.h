#ifndef DEF_DHCP_SNOOPING_H
#define DEF_DHCP_SNOOPING_H

/*
 * def_dhcp_snooping.h - DEFENCE dhcp_snooping (CONCEPT): the switch limits DHCP requests per port and checks the MAC inside them
 */

#include <stdint.h>

// Concept: 1 = the protection described in the README is in place.
int def_dhcp_snooping_holds(void);

#endif
