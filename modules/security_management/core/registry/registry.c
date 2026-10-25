/*
 * registry.c - The list of attacks and defences.
 *
 * Two plain arrays of pointers. The pairs register themselves once at
 * boot (core/catalog) and are never removed.
 *
 * Uses: strcmp (lib/string) to look them up by id.
 */

#include "registry.h"
#include "string.h"

static const attack_t  *attacks[REGISTRY_MAX_ATTACKS];
static const defence_t *defences[REGISTRY_MAX_DEFENCES];
static int attack_count;
static int defence_count;

void registry_add_attack(const attack_t *a) {
    if (a && attack_count < REGISTRY_MAX_ATTACKS)
        attacks[attack_count++] = a;
}

void registry_add_defence(const defence_t *d) {
    if (d && defence_count < REGISTRY_MAX_DEFENCES)
        defences[defence_count++] = d;
}

int registry_attack_count(void)  { return attack_count; }
int registry_defence_count(void) { return defence_count; }

const attack_t *registry_attack(int index) {
    if (index < 0 || index >= attack_count) return 0;
    return attacks[index];
}

const defence_t *registry_defence(int index) {
    if (index < 0 || index >= defence_count) return 0;
    return defences[index];
}

const attack_t *registry_find_attack(const char *id) {
    for (int i = 0; i < attack_count; i++)
        if (strcmp(attacks[i]->id, id) == 0)
            return attacks[i];
    return 0;
}

const defence_t *registry_find_defence(const char *id) {
    for (int i = 0; i < defence_count; i++)
        if (strcmp(defences[i]->id, id) == 0)
            return defences[i];
    return 0;
}

int registry_verify(const char *defence_id) {
    const defence_t *d = registry_find_defence(defence_id);
    if (!d || !d->selftest)
        return -1;
    return d->selftest() ? 1 : 0;
}

/* ---------- Names ---------- */

const char *sev_name(sev_t s) {
    switch (s) {
        case SEV_INFO:     return "info";
        case SEV_LOW:      return "low";
        case SEV_MEDIUM:   return "medium";
        case SEV_HIGH:     return "high";
        case SEV_CRITICAL: return "CRITICAL";
    }
    return "?";
}

const char *inv_name(invariant_t i) {
    switch (i) {
        case INV_RESOURCE:    return "resource";
        case INV_IDENTITY:    return "identity";
        case INV_ISOLATION:   return "isolation";
        case INV_PRIVILEGE:   return "privilege";
        case INV_LIVENESS:    return "liveness";
        case INV_CONSISTENCY: return "consistency";
        case INV_DISCLOSURE:  return "disclosure";
        case INV_TRUST:       return "trust";
        case INV_INTEGRITY:   return "integrity";
    }
    return "?";
}
