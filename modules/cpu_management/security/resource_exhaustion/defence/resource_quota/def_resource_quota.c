/*
 * def_resource_quota.c - DEFENCE resource_quota: every finite thing has
 *                        a per-process limit.
 *
 * Stops:  resource_exhaustion (security/resource_exhaustion)
 * Called: each limit is enforced at the place the resource is used -
 *         RLIMIT_CPU in the timer, NPROC in process_create, NOFILE in
 *         open(), FSIZE in write(), DATA in sbrk() (all core/rlimit).
 * Rule:   a process may not use more of a resource than its soft limit.
 *         A new process inherits the limits, so a quota set once covers
 *         every child.
 * Why:    without limits one process can take every slot, every file
 *         descriptor or all of memory and leave nothing for the rest.
 */

#include "modules/cpu_management/security/resource_exhaustion/defence/resource_quota/def_resource_quota.h"
#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/core/rlimit/rlimit.h"

int def_resource_quota_complete(void) {
    return rlimit_supported(RLIMIT_CPU)   && rlimit_supported(RLIMIT_NPROC)  &&
           rlimit_supported(RLIMIT_NOFILE)&& rlimit_supported(RLIMIT_FSIZE)  &&
           rlimit_supported(RLIMIT_DATA);
}

static int selftest(void) {
    if (!def_resource_quota_complete()) return 0;        // all five enforced
    if (rlimit_supported(RLIMIT_CORE)) return 0;         // UzOS has none to limit
    return 1;
}

static const defence_t defence = {
    "resource_quota",
    "CPU, procs, files, file size and heap each carry a per-process cap",
    "resource_exhaustion",
    selftest
};

void def_resource_quota_register(void) { registry_add_defence(&defence); }
