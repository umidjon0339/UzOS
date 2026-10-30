/*
 * def_optlen_check.c - DEFENCE optlen_check: read no option past its length.
 *
 * Stops:  sockopt_overflow (security/sockopt_overflow)
 * Called: socket_setopt() (core/socket), before any value is read.
 * Rule:   SO_RCVTIMEO / SO_SNDTIMEO need 8 bytes (a timeval), every
 *         other option an int (4); less is EINVAL. Only that much is read.
 * Why:    the length is the caller's claim; reading past it reads
 *         memory the caller never offered.
 *
 * Uses: socket option names (core/socket), audit_log (security_management).
 */

#include "modules/network_management/security/sockopt_overflow/defence/optlen_check/def_optlen_check.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                       // audit_log
#include "modules/network_management/core/socket/socket.h"                      // SO_*, socket_timeval_t

static uint32_t needed(int level, int name) {
    if (level == SOL_SOCKET && (name == SO_RCVTIMEO || name == SO_SNDTIMEO))
        return sizeof(socket_timeval_t);
    return sizeof(int);
}

int def_optlen_ok(int level, int name, uint32_t len) {
    if (len >= needed(level, name))
        return 1;
    audit_log("optlen_check", "setsockopt refused: the value is shorter than the option");
    return 0;
}

static int selftest(void) {
    return needed(SOL_SOCKET, SO_RCVBUF) == 4 && needed(SOL_SOCKET, SO_RCVTIMEO) == 8 && needed(SOL_IP, IP_TTL) == 4;
}

static const defence_t defence = {
    "optlen_check",
    "an option's length must be at least the size of the option",
    "sockopt_overflow",
    selftest
};

void def_optlen_check_register(void) { registry_add_defence(&defence); }
