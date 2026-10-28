/*
 * atk_resource_exhaustion.c - ATTACK resource_exhaustion: take it all.
 *
 * Breaks:     resource - every finite thing must be shared, not seized
 * Stopped by: resource_quota (defence/resource_quota)
 *
 * CPU time is only one finite thing. A process can also try to grab
 * every process slot, every file descriptor, or all of the heap. The
 * answer is the same for each: a per-process limit (rlimit), checked
 * where the resource is handed out, so no single process can take the
 * lot.
 *
 * simulate(): list the resources and where each limit is checked.
 * launch():   confirm the kernel has a limit for every one of them.
 * cleanup():  nothing.
 *
 * Uses: resource_quota (its defence), rlimit_* names (core/rlimit), ui.
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/resource_exhaustion/defence/resource_quota/def_resource_quota.h"
#include "modules/cpu_management/core/rlimit/rlimit.h"
#include "modules/ui/core/style/ui.h"

static const int watched[] = {
    RLIMIT_CPU, RLIMIT_NPROC, RLIMIT_NOFILE, RLIMIT_FSIZE, RLIMIT_DATA
};

static int simulate(void) {
    ui_printf("  a process can try to seize every finite resource:\n");
    for (unsigned i = 0; i < sizeof(watched) / sizeof(watched[0]); i++)
        ui_printf("    %-6s limited in %s\n",
                  rlimit_name(watched[i]), rlimit_enforced_in(watched[i]));
    return 1;
}

static int launch(void) {
    int findings = def_resource_quota_complete() ? 0 : 1;
    ui_printf("  every resource above carries a limit: %s\n",
              findings ? "NO (a gap!)" : "yes");
    return findings;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "resource_exhaustion",
    "seize every slot, descriptor or byte, not just the CPU",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "resource_quota"
};

void atk_resource_exhaustion_register(void) { registry_add_attack(&attack); }
