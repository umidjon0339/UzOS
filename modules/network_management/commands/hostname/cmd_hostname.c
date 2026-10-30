/*
 * cmd_hostname.c - `hostname`: what this machine is called.
 *
 *   hostname            uzos
 *   hostname lab1       rename it (needs CAP_NET_ADMIN); the prompt changes
 *   hostname -d         the domain name
 *   hostname -I         every address of this host, except lo's
 *
 * Uses: hostname_* (core/hostname), netdev_* (core/netdev), inet_format
 *       (core/inet), caps_require (security_management), arg_next,
 *       strcmp, ui_*.
 */

#include "modules/network_management/commands/hostname/cmd_hostname.h"  // this command
#include "modules/network_management/core/hostname/hostname.h"      // hostname_get, hostname_set
#include "modules/network_management/core/netdev/netdev.h"          // netdev_count, netdev_get
#include "modules/network_management/core/inet/inet.h"              // inet_format
#include "modules/security_management/core/caps/caps.h"             // caps_require, CAP_NET_ADMIN
#include "lib/args/args.h"                                          // arg_next
#include "lib/string/string.h"                                      // strcmp
#include "modules/ui/core/style/ui.h"                               // ui_*

void cmd_hostname(const char *args) {
    char word[HOSTNAME_MAX + 2];
    arg_next(args, word, sizeof(word));

    if (!word[0]) {
        ui_printf("%s\n", hostname_get());
    } else if (strcmp(word, "-d") == 0) {
        ui_printf("%s\n", domainname_get());
    } else if (strcmp(word, "-I") == 0) {
        char ip[INET_ADDR_TEXT];
        for (int i = 0; i < netdev_count(); i++) {
            netdev_t *dev = netdev_get(i);
            if (dev->ip && !(dev->flags & NETDEV_LOOPBACK)) {
                inet_format(dev->ip, ip);
                ui_printf("%s ", ip);
            }
        }
        ui_printf("\n");
    } else if (word[0] == '-') {
        ui_usage("hostname [-I | -d | newname]");
    } else if (caps_require(CAP_NET_ADMIN, "hostname")) {
        if (hostname_set(word) < 0)
            ui_error("hostname", "the name may only use letters, digits, '-' and '.' (1-63)");
    }
}
