/*
 * cmd_ifdown.c - `ifdown [eth0]`: release the address, then turn it off.
 *
 * The release goes out first, while the interface still works, so the
 * DHCP server may give the address to someone else at once.
 *
 * Uses: netdev_* (core/netdev), dhcp_release (core/dhcp), caps_require
 *       (security_management), arg_next, strcpy, ui_*.
 */

#include "modules/network_management/commands/ifdown/cmd_ifdown.h"  // this command
#include "modules/network_management/core/netdev/netdev.h"          // netdev_by_name, netdev_set_up
#include "modules/network_management/core/dhcp/dhcp.h"              // dhcp_release
#include "modules/security_management/core/caps/caps.h"             // caps_require, CAP_NET_ADMIN
#include "lib/args/args.h"                                          // arg_next
#include "lib/string/string.h"                                      // strcpy
#include "modules/ui/core/style/ui.h"                               // ui_*

void cmd_ifdown(const char *args) {
    char name[NETDEV_NAME_LEN] = "eth0";
    char word[NETDEV_NAME_LEN];
    arg_next(args, word, sizeof(word));
    if (word[0])
        strcpy(name, word);
    netdev_t *dev = netdev_by_name(name);
    if (!dev) {
        ui_usage("ifdown [eth0]");
        return;
    }
    if (!caps_require(CAP_NET_ADMIN, "ifdown"))
        return;
    uint32_t lock = netdev_lock();
    if (!(dev->flags & NETDEV_LOOPBACK))
        dhcp_release();                         // say goodbye while we still can
    netdev_set_up(dev, 0);
    netdev_unlock(lock);
    ui_printf("%s: down\n", name);
}
