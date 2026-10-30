/*
 * def_response_limit.c - DEFENCE response_limit (CONCEPT): a server limits answers per client and never answers much bigger than asked.
 *
 * Stops:  amplification_reflection (security/amplification_reflection)
 * Called: nothing in UzOS (echod answers byte for byte).
 * Rule:   response rate limiting (RRL), no answer bigger than N x the question to an unverified client.
 * Why:    the victim never asked; the server must not shout at it.
 *
 * Uses: nothing in UzOS (see the README).
 */

#include "modules/network_management/security/amplification_reflection/defence/response_limit/def_response_limit.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence

int def_response_limit_holds(void) { return 1; }     // kept outside UzOS: see the README

static int selftest(void) {
    return def_response_limit_holds();
}

static const defence_t defence = {
    "response_limit",
    "a server limits answers per client and never answers much bigger than asked",
    "amplification_reflection",
    selftest
};

void def_response_limit_register(void) { registry_add_defence(&defence); }
