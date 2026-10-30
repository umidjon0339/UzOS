#ifndef DEF_RMEM_LIMIT_H
#define DEF_RMEM_LIMIT_H

/*
 * def_rmem_limit.h - DEFENCE rmem_limit: a socket's queue is charged a whole buffer per datagram, up to its rcvbuf
 */

#include <stdint.h>

// May a datagram costing 'truesize' join a queue already charged
// 'charged', with a limit of 'rcvbuf'? socket_queue() asks.
// 1 = yes, 0 = drop it (logged).
int def_rmem_ok(uint32_t charged, uint32_t truesize, uint32_t rcvbuf);

#endif
