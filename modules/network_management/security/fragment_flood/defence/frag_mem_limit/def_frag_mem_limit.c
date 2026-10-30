/*
 * def_frag_mem_limit.c - DEFENCE frag_mem_limit: a fair share of the reassembly slots.
 *
 * Stops:  fragment_flood (security/fragment_flood)
 * Called: ipv4_reassemble() (core/ipv4/fragment.c), before a new slot is taken.
 * Rule:   one source address may hold at most FRAG_SOURCE_MAX slots.
 *         (Every slot also expires after 30 s.)
 * Why:    the memory for half-built datagrams is shared; Linux bounds it
 *         too (ipfrag_high_thresh).
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/fragment_flood/defence/frag_mem_limit/def_frag_mem_limit.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int ok(int held) {
    return held >= 0 && held < FRAG_SOURCE_MAX;
}

int def_frag_slot_ok(int held) {
    if (ok(held))
        return 1;
    audit_log("frag_mem_limit", "fragment refused: this source holds its share of reassembly slots");
    return 0;
}

static int selftest(void) {
    return ok(0) && ok(FRAG_SOURCE_MAX - 1) && !ok(FRAG_SOURCE_MAX) && !ok(-1);
}

static const defence_t defence = {
    "frag_mem_limit",
    "one source may hold at most 2 of the 4 reassembly slots",
    "fragment_flood",
    selftest
};

void def_frag_mem_limit_register(void) { registry_add_defence(&defence); }
