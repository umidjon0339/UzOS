#ifndef DEF_IDLE_TIMEOUT_H
#define DEF_IDLE_TIMEOUT_H

/*
 * def_idle_timeout.h - DEFENCE idle_timeout: a request must be complete within 5 s in total, not per read
 */

#include <stdint.h>

#define REQUEST_DEADLINE_MS 5000

// Is a request that began at 'started_ms' still within its deadline?
// httpd asks before each read. 1 = go on, 0 = drop the client (logged).
int def_request_time_ok(uint32_t started_ms);

#endif
