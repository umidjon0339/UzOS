/*
 * atk_amplification_reflection.c - ATTACK amplification_reflection (CONCEPT): use servers as amplifiers (concept)
 *
 * Breaks:     resource - small questions do not make huge answers to a forged address
 * Stopped by: response_limit (defence/response_limit) - CONCEPT
 *
 * A 60-byte DNS question ("ANY isc.org") can earn a 4000-byte answer;
 * memcached answered 10000x. With the victim as the forged source, many
 * servers flood it. UzOS serves only echo, whose answer is exactly as big
 * as the question.
 *
 * Nothing is launched; the defence is described, not run.
 *
 * Uses: def_response_limit_holds (its defence), ui_printf.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/amplification_reflection/defence/response_limit/def_response_limit.h" // def_response_limit_holds
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static int simulate(void) {
    ui_printf("  a small question with a forged source -> a big answer to the victim\n");
    ui_printf("  DNS x50, NTP monlist x500, memcached x10000\n");
    ui_printf("  UzOS's echod answers 1:1: no amplification here\n");
    return 1;
}

static int launch(void) {
    ui_printf("  not run: a concept (see the README)\n");
    return def_response_limit_holds() ? 0 : 1;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "amplification_reflection",
    "small requests with a forged source make servers send big answers to a victim",
    INV_RESOURCE, SEV_HIGH,
    simulate, launch, cleanup,
    "response_limit"
};

void atk_amplification_reflection_register(void) { registry_add_attack(&attack); }
