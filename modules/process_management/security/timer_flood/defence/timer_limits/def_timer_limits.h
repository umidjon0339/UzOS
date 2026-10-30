#ifndef DEF_TIMER_LIMITS_H
#define DEF_TIMER_LIMITS_H

#include <stdint.h>
#include "modules/process_management/core/process/process.h"   // process_t

#define TIMER_USER_MAX 8        // periodic timers armed at once, per user

// May 'p' arm a PERIODIC interval timer ('which' = ITIMER_*)? Its user may
// have at most TIMER_USER_MAX armed in all its processes (CAP_RESOURCE:
// no limit). A one-shot timer is always fine. setitimer() asks this.
// 1 = allowed, 0 = refused (logged).
int def_itimer_allowed(const process_t *p, int which);

#endif
