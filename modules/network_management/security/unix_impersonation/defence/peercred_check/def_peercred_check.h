#ifndef DEF_PEERCRED_CHECK_H
#define DEF_PEERCRED_CHECK_H

/*
 * def_peercred_check.h - DEFENCE peercred_check: SO_PEERCRED reports the connecting socket's owner, recorded by the kernel at connect()
 */

#include <stdint.h>

#include "modules/network_management/core/socket/socket.h"     // socket_cred_t

// The credentials of socket 's', as the kernel records them for its peer.
socket_cred_t def_peer_cred(const struct socket *s);

#endif
