/*
 * def_tcp_flag_sanity.c - DEFENCE tcp_flag_sanity: nonsense flags get no answer.
 *
 * Stops:  xmas_null_scan (security/xmas_null_scan)
 * Called: tcp_receive() (core/tcp), before the connection is looked up.
 * Rule:   drop (silently) a segment with no flags at all, with SYN+FIN,
 *         SYN+RST, or FIN+PSH+URG without ACK.
 * Why:    no real stack sends them; only scanners do.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/xmas_null_scan/defence/tcp_flag_sanity/def_tcp_flag_sanity.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

static int sane(uint8_t flags) {
    uint8_t f = flags & 0x3F;                       // FIN SYN RST PSH ACK URG
    if (f == 0)
        return 0;                                   // NULL scan
    if ((f & 0x03) == 0x03 || (f & 0x06) == 0x06)
        return 0;                                   // SYN+FIN, SYN+RST
    if ((f & 0x29) == 0x29 && !(f & 0x10))
        return 0;                                   // Xmas: FIN+PSH+URG, no ACK
    return 1;
}

int def_tcp_flags_ok(uint8_t flags) {
    if (sane(flags))
        return 1;
    audit_log("tcp_flag_sanity", "dropped a TCP segment with impossible flags (scan?)");
    return 0;
}

static int selftest(void) {
    return sane(0x02) && sane(0x12) && sane(0x10) && sane(0x11) && sane(0x18) && !sane(0) && !sane(0x03) && !sane(0x06) && !sane(0x29);
}

static const defence_t defence = {
    "tcp_flag_sanity",
    "segments with impossible flag combinations are dropped silently",
    "xmas_null_scan",
    selftest
};

void def_tcp_flag_sanity_register(void) { registry_add_defence(&defence); }
