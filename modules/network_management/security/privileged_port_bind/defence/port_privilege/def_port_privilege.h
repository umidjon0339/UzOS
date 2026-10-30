#ifndef DEF_PORT_PRIVILEGE_H
#define DEF_PORT_PRIVILEGE_H

/*
 * def_port_privilege.h - DEFENCE port_privilege: binding a port below 1024 needs CAP_NET_BIND
 */

#include <stdint.h>

#define PORT_PRIVILEGED 1024

// May 'uid' bind 'port'? The socket bind functions ask. 1 = yes, 0 = no (logged).
int def_port_bind_ok(uint32_t uid, uint16_t port);

#endif
