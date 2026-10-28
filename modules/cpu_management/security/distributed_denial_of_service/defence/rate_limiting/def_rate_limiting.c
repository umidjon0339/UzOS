/*
 * def_rate_limiting.c - DEFENCE rate_limiting (CONCEPT): a distributed
 *                       flood is stopped at the network edge.
 *
 * Stops:  distributed_denial_of_service (CONCEPT - see README)
 * Called: nothing in UzOS: there is no network stack, so there is no
 *         remote flood to meet. On a real server the defence is rate
 *         limiting, load balancing and upstream traffic filtering.
 * Rule:   requests per source per second are capped; excess is dropped
 *         before it reaches the CPU. (Described, not implemented here.)
 * Why:    honesty - a DDoS needs many machines and a network; UzOS is
 *         one machine with no network, so it cannot be a target. The
 *         single-machine case (hogging the CPU) is denial_of_service.
 */

#include "modules/cpu_management/security/distributed_denial_of_service/defence/rate_limiting/def_rate_limiting.h"
#include "modules/security_management/core/registry/registry.h"

int def_no_network_surface(void) { return 1; }   // no network stack exists

static int selftest(void) { return def_no_network_surface(); }

static const defence_t defence = {
    "rate_limiting",
    "concept: no network stack = no remote flood; edge rate-limits a real one",
    "distributed_denial_of_service",
    selftest
};

void def_rate_limiting_register(void) { registry_add_defence(&defence); }
