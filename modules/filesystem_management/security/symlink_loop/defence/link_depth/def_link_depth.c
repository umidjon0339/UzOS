/*
 * def_link_depth.c - DEFENCE link_depth: a path walk follows at most 8 links.
 *
 * Stops:  symlink_loop (security/symlink_loop)
 * Called: vfs_lookup (core/vfs), before it follows each symbolic link.
 * Rule:   one path walk follows at most LINK_HOPS_MAX symbolic links;
 *         the next one fails with ELOOP.
 * Why:    `ln -s a b ; ln -s b a ; cat a` would follow a -> b -> a ...
 *         for ever, inside the kernel. And each link is walked by calling
 *         the walker again: a long chain would also run off the 4 KiB
 *         kernel stack. A fixed number of hops ends both. (Linux: 40,
 *         and at most 8 nested.)
 *
 * Uses: audit_log, registry.
 */
#include "modules/filesystem_management/security/symlink_loop/defence/link_depth/def_link_depth.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log

static int allowed(int hops) {
    return hops >= 1 && hops <= LINK_HOPS_MAX;
}

int def_link_hop_ok(int hops) {
    if (allowed(hops))
        return 1;
    audit_log("link_depth", "too many symbolic links in one path (ELOOP)");
    return 0;
}

static int selftest(void) {
    if (!allowed(1)) return 0;
    if (!allowed(LINK_HOPS_MAX)) return 0;
    if (allowed(LINK_HOPS_MAX + 1)) return 0;   // a loop runs into this
    return 1;
}

static const defence_t defence = {
    "link_depth",
    "a path walk follows at most 8 symbolic links (ELOOP)",
    "symlink_loop",
    selftest
};

void def_link_depth_register(void) { registry_add_defence(&defence); }
