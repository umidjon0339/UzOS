/*
 * def_ttl_normalize.c - DEFENCE ttl_normalize (ILLUSTRATION): send what everybody sends.
 *
 * Stops:  ttl_recon (security/ttl_recon)
 * Called: ipv4_send() (core/ipv4), when the caller gave no TTL.
 * Rule:   64 (the Linux default); a program may still choose its own.
 * Why:    an unusual default makes a machine stand out in a scan.
 *
 * Uses: nothing.
 */

#include "modules/network_management/security/ttl_recon/defence/ttl_normalize/def_ttl_normalize.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence

uint8_t def_default_ttl(void) { return TTL_COMMON; }

static int selftest(void) {
    return def_default_ttl() == 64;
}

static const defence_t defence = {
    "ttl_normalize",
    "the default TTL is the common 64",
    "ttl_recon",
    selftest
};

void def_ttl_normalize_register(void) { registry_add_defence(&defence); }
