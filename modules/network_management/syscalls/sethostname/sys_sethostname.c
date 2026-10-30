/*
 * sys_sethostname.c - sethostname(name, len): set the hostname.
 *
 * The name is NOT a C string: it is 'len' bytes (Linux's rule), copied
 * into a buffer of our own and only then checked. Only root may do it
 * (CAP_NET_ADMIN), since every user's prompt shows it.
 *
 * Uses: hostname_set (core/hostname), caps_caller_has (security_management),
 *       memcpy (lib/string).
 */
#include "modules/network_management/syscalls/sethostname/sys_sethostname.h"  // this file
#include "modules/network_management/core/hostname/hostname.h"              // hostname_set, HOSTNAME_MAX
#include "modules/network_management/core/inet/inet.h"                      // NET_EPERM, NET_EINVAL
#include "modules/security_management/core/caps/caps.h"                     // caps_caller_has, CAP_NET_ADMIN
#include "lib/string/string.h"                                              // memcpy

int sys_sethostname(const char *name, uint32_t len) {
    if (!caps_caller_has(CAP_NET_ADMIN))
        return -NET_EPERM;
    if (len == 0 || len >= HOSTNAME_MAX)
        return -NET_EINVAL;
    char copy[HOSTNAME_MAX];
    memcpy(copy, name, len);
    copy[len] = '\0';
    return hostname_set(copy);
}
