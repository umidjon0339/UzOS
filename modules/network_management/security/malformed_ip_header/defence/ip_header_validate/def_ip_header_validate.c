/*
 * def_ip_header_validate.c - DEFENCE ip_header_validate: an IP header is checked before anything reads it.
 *
 * Stops:  malformed_ip_header (security/malformed_ip_header)
 * Called: ipv4_receive() (core/ipv4), first thing.
 * Rule:   version 4; header length (IHL x 4) >= 20 and <= total length;
 *         total length <= the bytes received; checksum correct.
 * Why:    every later read trusts these numbers.
 *
 * Uses: inet_checksum (core/inet), audit_log (security_management).
 */

#include "modules/network_management/security/malformed_ip_header/defence/ip_header_validate/def_ip_header_validate.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/network_management/core/inet/inet.h"                          // inet_checksum, ntohs
#include "modules/network_management/core/ipv4/ipv4.h"                          // ipv4_header_t, IPV4_HLEN

static int ok(const uint8_t *bytes, uint32_t len) {
    if (len < IPV4_HLEN)
        return 0;
    const ipv4_header_t *ip = (const ipv4_header_t *)bytes;
    uint32_t hlen = (uint32_t)(ip->version_ihl & 0x0F) * 4;
    uint32_t total = ntohs(ip->total_length);
    if ((ip->version_ihl >> 4) != 4 || hlen < IPV4_HLEN || hlen > total || total > len)
        return 0;
    return inet_checksum(ip, hlen) == 0;
}

int def_ip_header_ok(const uint8_t *bytes, uint32_t len) {
    if (ok(bytes, len))
        return 1;
    audit_log("ip_header_validate", "dropped a packet with a broken IP header");
    return 0;
}

static int selftest(void) {
    uint8_t h[IPV4_HLEN] = { 0x45, 0, 0, 20, 0, 0, 0, 0, 64, 1, 0, 0, 10, 0, 2, 2, 10, 0, 2, 15 };
    uint16_t sum = inet_checksum(h, IPV4_HLEN);
    h[10] = (uint8_t)sum;
    h[11] = (uint8_t)(sum >> 8);
    if (!ok(h, 20)) return 0;                   // a good header
    if (ok(h, 19)) return 0;                    // cut short
    h[0] = 0x44;
    if (ok(h, 20)) return 0;                    // IHL 4: 16 bytes
    return 1;
}

static const defence_t defence = {
    "ip_header_validate",
    "version 4, 20 <= IHL*4 <= total length <= packet, and a correct checksum",
    "malformed_ip_header",
    selftest
};

void def_ip_header_validate_register(void) { registry_add_defence(&defence); }
