/*
 * def_ecc_and_refresh.c - DEFENCE ecc_and_refresh (CONCEPT): DRAM bit
 *                         flips are a hardware problem.
 *
 * Stops:  rowhammer (CONCEPT - see README)
 * Called: nothing in UzOS: emulated memory does not leak charge, and
 *         the real fixes live in the memory controller (ECC, refresh,
 *         Target Row Refresh), not in the operating system.
 * Rule:   corrected/detected bit flips (ECC) and frequent refresh keep
 *         a hammered row from flipping a neighbour. (Described only.)
 * Why:    honesty - Rowhammer needs physical DRAM; QEMU cannot
 *         reproduce it, so there is nothing for the OS to enforce.
 */

#include "modules/cpu_management/security/rowhammer/defence/ecc_and_refresh/def_ecc_and_refresh.h"
#include "modules/security_management/core/registry/registry.h"

int def_memory_not_vulnerable(void) { return 1; }   // emulated RAM, no charge leak

static int selftest(void) { return def_memory_not_vulnerable(); }

static const defence_t defence = {
    "ecc_and_refresh",
    "concept: emulated RAM cannot flip bits; ECC/refresh fix real DRAM",
    "rowhammer",
    selftest
};
void def_ecc_and_refresh_register(void) { registry_add_defence(&defence); }
