/*
 * atk_distributed_denial_of_service.c - ATTACK ddos (CONCEPT).
 *
 * Breaks:     liveness - a service must stay reachable
 * Stopped by: rate_limiting (defence/rate_limiting) - CONCEPT
 *
 * A DDoS uses many machines to flood one service until its CPU (or
 * network) cannot serve real users. It needs a network and many
 * attackers. UzOS is a single machine with no network stack, so it
 * CANNOT be a DDoS target and nothing is launched here. The
 * single-machine version - one process hogging the CPU - is the
 * denial_of_service pair. See the README.
 *
 * Uses: rate_limiting (its defence), ui_printf (ui).
 */

#include "modules/security_management/core/registry/registry.h"
#include "modules/cpu_management/security/distributed_denial_of_service/defence/rate_limiting/def_rate_limiting.h"
#include "modules/ui/core/style/ui.h"

static int simulate(void) {
    ui_printf("  many machines flood one service until its CPU gives out\n");
    ui_printf("  defence: rate limiting + load balancing at the edge\n");
    ui_printf("  UzOS has no network stack, so it cannot be a target\n");
    return 1;
}
static int launch(void) {
    ui_printf("  nothing to launch: no network surface in UzOS\n");
    return def_no_network_surface() ? 0 : 1;
}
static int cleanup(void) { return 0; }

static const attack_t attack = {
    "distributed_denial_of_service",
    "many machines flood one service (concept: UzOS has no network)",
    INV_LIVENESS, SEV_INFO,
    simulate, launch, cleanup,
    "rate_limiting"
};
void atk_distributed_denial_of_service_register(void) { registry_add_attack(&attack); }
