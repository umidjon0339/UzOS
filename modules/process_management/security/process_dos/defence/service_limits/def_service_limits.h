#ifndef DEF_SERVICE_LIMITS_H
#define DEF_SERVICE_LIMITS_H

// Denial of service through process abuse - spawning endlessly, hogging
// the CPU - is held off by LIMITS: a per-user process cap (slot_quota /
// RLIMIT_NPROC) and CPU quotas (cpu_management). 1 if those limits are
// the answer here (they are); the real enforcement lives in those pairs.
int def_service_limits_apply(void);

#endif
