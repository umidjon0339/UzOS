/*
 * sys_setdomainname.c - setdomainname(name, len): set the domain name.
 *
 * The same rules as sethostname: 'len' bytes, root only.
 *
 * Uses: domainname_set (core/hostname), caps_caller_has (security_management),
 *       memcpy (lib/string).
 */
#include "modules/network_management/syscalls/setdomainname/sys_setdomainname.h"  // this file
#include "modules/network_management/core/hostname/hostname.h"              // domainname_set, HOSTNAME_MAX
#include "modules/network_management/core/inet/inet.h"                      // NET_EPERM, NET_EINVAL
#include "modules/security_management/core/caps/caps.h"                     // caps_caller_has, CAP_NET_ADMIN
#include "lib/string/string.h"                                              // memcpy

int sys_setdomainname(const char *name, uint32_t len) {
    if (!caps_caller_has(CAP_NET_ADMIN))
        return -NET_EPERM;
    if (len == 0 || len >= HOSTNAME_MAX)
        return -NET_EINVAL;
    char copy[HOSTNAME_MAX];
    memcpy(copy, name, len);
    copy[len] = '\0';
    return domainname_set(copy);
}
