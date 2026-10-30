#ifndef DEF_CONN_QUOTA_H
#define DEF_CONN_QUOTA_H

/*
 * def_conn_quota.h - DEFENCE conn_quota: a normal user may own at most 8 TCP connections at once
 */

#include <stdint.h>

#define TCP_USER_MAX 8

// May 'uid', who owns 'owned' connections, make one more? tcp_new()
// asks. 1 = yes, 0 = no (logged).
int def_conn_ok(uint32_t uid, int owned);

#endif
