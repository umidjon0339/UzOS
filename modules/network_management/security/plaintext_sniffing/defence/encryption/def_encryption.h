#ifndef DEF_ENCRYPTION_H
#define DEF_ENCRYPTION_H

/*
 * def_encryption.h - DEFENCE encryption (ILLUSTRATION): secrets travel only inside an encrypted channel (TLS, SSH)
 */

#include <stdint.h>

// 1 if 'port' is a protocol that sends logins in plaintext (21, 23, 80,
// 110, 143): a warning label, since UzOS cannot encrypt.
int def_plaintext_port(uint16_t port);

#endif
