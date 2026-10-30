#ifndef DEF_REUSEPORT_OWNER_H
#define DEF_REUSEPORT_OWNER_H

/*
 * def_reuseport_owner.h - DEFENCE reuseport_owner: SO_REUSEPORT sockets may share a port only with the same user's
 */

#include <stdint.h>

// May a socket of 'uid' (reuseport 'wants') join a port held by a socket
// of 'owner' (reuseport 'has')? udp_bind() asks. 1 = yes, 0 = no (logged
// when only the owner differs).
int def_reuseport_ok(uint32_t uid, int wants, uint32_t owner, int has);

#endif
