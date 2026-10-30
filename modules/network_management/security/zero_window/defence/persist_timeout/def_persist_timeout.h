#ifndef DEF_PERSIST_TIMEOUT_H
#define DEF_PERSIST_TIMEOUT_H

/*
 * def_persist_timeout.h - DEFENCE persist_timeout: a window that stays 0 for 4 probes ends the connection
 */

#include <stdint.h>

#define PERSIST_MAX_PROBES 4

// May we send window probe number 'probes' (counted from 1)?
// tcp_timer asks. 1 = yes, 0 = give up on the connection (logged).
int def_persist_ok(int probes);

#endif
