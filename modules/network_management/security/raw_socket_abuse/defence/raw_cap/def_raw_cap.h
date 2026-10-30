#ifndef DEF_RAW_CAP_H
#define DEF_RAW_CAP_H

/*
 * def_raw_cap.h - DEFENCE raw_cap: SOCK_RAW needs CAP_NET_RAW
 */

#include <stdint.h>

// May 'uid' open a raw socket? socket_create() asks. 1 = yes, 0 = EPERM (logged).
int def_raw_socket_ok(uint32_t uid);

#endif
