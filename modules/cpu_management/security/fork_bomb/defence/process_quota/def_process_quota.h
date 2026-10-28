#ifndef DEF_PROCESS_QUOTA_H
#define DEF_PROCESS_QUOTA_H

#include <stdint.h>

// May a user who already owns 'count' processes start one more, given a
// per-user RLIMIT_NPROC of 'limit'? 'is_root' holders (CAP_RESOURCE)
// are exempt. The kernel asks this in process_create(), through
// rlimit_nproc_ok() (core/rlimit): 1 = allowed, 0 = refused.
int def_nproc_quota_ok(int is_root, uint32_t count, uint32_t limit);

#endif
