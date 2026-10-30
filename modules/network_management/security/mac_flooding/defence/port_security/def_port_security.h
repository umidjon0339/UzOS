#ifndef DEF_PORT_SECURITY_H
#define DEF_PORT_SECURITY_H

/*
 * def_port_security.h - DEFENCE port_security (CONCEPT): a switch port accepts only a few MACs (port security)
 */

#include <stdint.h>

// Concept: 1 = the protection described in the README is in place.
int def_port_security_holds(void);

#endif
