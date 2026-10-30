/*
 * def_encryption.c - DEFENCE encryption (ILLUSTRATION): encryption (illustration: UzOS has none).
 *
 * Stops:  plaintext_sniffing (security/plaintext_sniffing)
 * Called: nothing in UzOS: it has no TLS or SSH.
 * Rule:   never send a secret over a plaintext protocol; use TLS (https,
 *         ftps) or SSH instead of http, ftp, telnet.
 * Why:    sniffing needs only a place on the path; encryption is the only
 *         thing that makes the bytes useless there.
 *
 * Uses: nothing.
 */

#include "modules/network_management/security/plaintext_sniffing/defence/encryption/def_encryption.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence

int def_plaintext_port(uint16_t port) {
    return port == 21 || port == 23 || port == 80 || port == 110 || port == 143;
}

static int selftest(void) {
    return def_plaintext_port(23) && def_plaintext_port(80) && !def_plaintext_port(22) && !def_plaintext_port(443);
}

static const defence_t defence = {
    "encryption",
    "secrets travel only inside an encrypted channel (TLS, SSH)",
    "plaintext_sniffing",
    selftest
};

void def_encryption_register(void) { registry_add_defence(&defence); }
