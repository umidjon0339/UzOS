#ifndef DEF_SIGQUEUE_LIMIT_H
#define DEF_SIGQUEUE_LIMIT_H

#include <stdint.h>

#define SIGQUEUE_USER_MAX 16    // a quarter of the pool, whatever the rlimit says

// May user 'uid', who already has 'queued' signals waiting in the pool,
// queue one more? Its limit is RLIMIT_SIGPENDING ('rlimit'), and never
// more than SIGQUEUE_USER_MAX. sigqueue_add() (core/signal) asks this
// before it takes a pool entry. 1 = allowed, 0 = refused (logged).
int def_sigqueue_allowed(uint32_t uid, int queued, uint32_t rlimit);

#endif
