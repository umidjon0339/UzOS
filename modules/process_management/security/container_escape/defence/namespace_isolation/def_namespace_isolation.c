/*
 * def_namespace_isolation.c - DEFENCE namespace_isolation (concept).
 *
 * Stops:  container_escape (security/container_escape)
 * Called: nothing in UzOS: there are no namespaces or containers.
 * Rule:   a container sees only its own PIDs, mounts, users and network,
 *         through Linux namespaces; combined with cgroups and dropped
 *         capabilities, that keeps it from reaching the host.
 * Why:    honesty - "container escape" needs containers. UzOS has one
 *         flat system, so there is nothing to break out of; this records
 *         what a real system uses to isolate them.
 */

#include "modules/process_management/security/container_escape/defence/namespace_isolation/def_namespace_isolation.h"  // this file
#include "modules/security_management/core/registry/registry.h"                                                        // defence_t, registry_add_defence

int def_namespaces_present(void) { return 1; }   // documented concept

static int selftest(void) { return def_namespaces_present(); }

static const defence_t defence = {
    "namespace_isolation",
    "concept: namespaces + cgroups + dropped caps isolate a container",
    "container_escape",
    selftest
};

void def_namespace_isolation_register(void) { registry_add_defence(&defence); }
