/*
 * def_nlink_bounds.c - DEFENCE nlink_bounds: the link count has a ceiling.
 *
 * Stops:  nlink_overflow (security/nlink_overflow)
 * Called: ramdisk_link() (core/ramdisk), before nlink is counted up.
 * Rule:   a file has at most FS_LINK_MAX names; the next link() fails
 *         with EMLINK.
 * Why:    nlink decides when a file's memory is freed: at 0 names (and
 *         no open descriptor). A counter that can be pushed up for ever
 *         wraps around - on file systems with 16-bit counts this was a
 *         real bug: 65536 links, the count reads 0, the next unlink frees
 *         a file that 65535 names still point at. A ceiling well below
 *         the counter's limit makes the wrap impossible.
 *
 * Uses: audit_log, registry.
 */
#include "modules/filesystem_management/security/nlink_overflow/defence/nlink_bounds/def_nlink_bounds.h"  // this file
#include "modules/security_management/core/registry/registry.h"   // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"         // audit_log

static int allowed(uint32_t nlink) {
    return nlink >= 1 && nlink < FS_LINK_MAX;
}

int def_nlink_ok(uint32_t nlink) {
    if (allowed(nlink))
        return 1;
    audit_log("nlink_bounds", "hard link refused: the file has too many names (EMLINK)");
    return 0;
}

static int selftest(void) {
    if (!allowed(1)) return 0;
    if (!allowed(FS_LINK_MAX - 1)) return 0;    // the last one
    if (allowed(FS_LINK_MAX)) return 0;         // one too many
    if (allowed(0xFFFFFFFFu)) return 0;
    return 1;
}

static const defence_t defence = {
    "nlink_bounds",
    "a file has at most 16 names: the link count cannot wrap",
    "nlink_overflow",
    selftest
};

void def_nlink_bounds_register(void) { registry_add_defence(&defence); }
