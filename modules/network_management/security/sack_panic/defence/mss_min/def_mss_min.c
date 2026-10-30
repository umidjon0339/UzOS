/*
 * def_mss_min.c - DEFENCE mss_min: a floor for the peer's MSS.
 *
 * Stops:  sack_panic (security/sack_panic)
 * Called: tcp.c, wherever a SYN's MSS option is read (new_child, syn_sent).
 * Rule:   use max(MSS, TCP_MSS_MIN); log when it had to be raised.
 * Why:    Linux's tcp_min_snd_mss (48), added for CVE-2019-11477.
 *
 * Uses: audit_log (security_management).
 */

#include "modules/network_management/security/sack_panic/defence/mss_min/def_mss_min.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log

uint16_t def_mss_clamp(uint16_t mss) {
    if (mss >= TCP_MSS_MIN)
        return mss;
    audit_log("mss_min", "a peer announced a tiny MSS: raised to 48 (SACK Panic?)");
    return TCP_MSS_MIN;
}

static int selftest(void) {
    return def_mss_clamp(1460) == 1460 && def_mss_clamp(48) == 48 && def_mss_clamp(8) == 48;
}

static const defence_t defence = {
    "mss_min",
    "a peer's MSS below 48 is raised to 48",
    "sack_panic",
    selftest
};

void def_mss_min_register(void) { registry_add_defence(&defence); }
