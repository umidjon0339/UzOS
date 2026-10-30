/*
 * hostname.c - Keep the host and domain name; refuse names that are not names.
 *
 * A name ends up in prompts, in log lines and on the network, so only
 * plain characters may be in it: a name with control characters could
 * repaint the screen of whoever prints it.
 *
 * Uses: NET_EINVAL (core/inet), strcpy / strlen (lib/string).
 */

#include "modules/network_management/core/hostname/hostname.h"   // this file
#include "modules/network_management/core/inet/inet.h"           // NET_EINVAL
#include "lib/string/string.h"                                    // strcpy, strlen

static char host[HOSTNAME_MAX] = "uzos";
static char domain[HOSTNAME_MAX] = "(none)";

const char *hostname_get(void)   { return host; }
const char *domainname_get(void) { return domain; }

static int valid(const char *name) {
    int len = (int)strlen(name);
    if (len < 1 || len >= HOSTNAME_MAX)
        return 0;
    for (int i = 0; i < len; i++) {
        char c = name[i];
        int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                 (c >= '0' && c <= '9') || c == '-' || c == '.';
        if (!ok)
            return 0;
    }
    return 1;
}

int hostname_set(const char *name) {
    if (!valid(name))
        return -NET_EINVAL;
    strcpy(host, name);
    return 0;
}

int domainname_set(const char *name) {
    if (!valid(name))
        return -NET_EINVAL;
    strcpy(domain, name);
    return 0;
}
