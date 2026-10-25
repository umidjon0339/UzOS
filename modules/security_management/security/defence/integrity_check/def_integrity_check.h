#ifndef DEF_INTEGRITY_CHECK_H
#define DEF_INTEGRITY_CHECK_H

#include <stdint.h>
#include "sha256.h"

// Is the hash measured 'now' the one measured at boot?
// 1 = unchanged, 0 = changed (logged with 'alarm' as the detail).
int def_integrity_unchanged(const char *alarm, const uint8_t at_boot[SHA256_SIZE],
                            const uint8_t now[SHA256_SIZE]);

#endif
