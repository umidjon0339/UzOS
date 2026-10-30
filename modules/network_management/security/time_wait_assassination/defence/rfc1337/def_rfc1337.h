#ifndef DEF_RFC1337_H
#define DEF_RFC1337_H

/*
 * def_rfc1337.h - DEFENCE rfc1337: a RST never ends TIME_WAIT early
 */

#include <stdint.h>

// May a RST end a connection in TIME_WAIT? segment() asks.
// Always 0 (logged).
int def_timewait_rst_ok(void);

#endif
