/*
 * cmd_ifup.c - `ifup [eth0]`: turn the interface on, then ask DHCP.
 *
 * Linux's ifup reads /etc/network/interfaces to know what to do; UzOS
 * has one answer: "up, then DHCP". It is the opposite of ifdown.
 *
 * Uses: netdev_* (core/netdev), dhcp_* (core/dhcp), dns_set_nameserver
 *       (core/dns), inet_format (core/inet), caps_require
 *       (security_management), sys_sleep_interruptible / signal_interrupted
 *       (process_management), arg_next, strcpy, ui_*.
 */

#include "modules/network_management/commands/ifup/cmd_ifup.h"      // this command
#include "modules/network_management/core/netdev/netdev.h"          // netdev_by_name, netdev_set_up
#include "modules/network_management/core/dhcp/dhcp.h"              // dhcp_start, dhcp_lease
#include "modules/network_management/core/dns/dns.h"                // dns_set_nameserver
#include "modules/network_management/core/inet/inet.h"              // inet_format, inet_prefix
#include "modules/security_management/core/caps/caps.h"             // caps_require, CAP_NET_ADMIN
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"          // signal_interrupted
#include "lib/args/args.h"                                          // arg_next
#include "lib/string/string.h"                                      // strcpy
#include "modules/ui/core/style/ui.h"                               // ui_*

#define WAIT_MS 10000

void cmd_ifup(const char *args) {
    char name[NETDEV_NAME_LEN] = "eth0";
    char word[NETDEV_NAME_LEN];
    arg_next(args, word, sizeof(word));
    if (word[0])
        strcpy(name, word);
    netdev_t *dev = netdev_by_name(name);
    if (!dev || (dev->flags & NETDEV_LOOPBACK)) {
        ui_usage("ifup [eth0]");
        return;
    }
    if (!caps_require(CAP_NET_ADMIN, "ifup"))
        return;

    uint32_t lock = netdev_lock();
    netdev_set_up(dev, 1);
    dhcp_start(dev);
    netdev_unlock(lock);
    ui_printf("%s: up, asking DHCP...\n", name);

    const dhcp_lease_t *l = dhcp_lease();
    for (int ms = 0; ms < WAIT_MS && l->state != DHCP_BOUND && l->state != DHCP_FAILED &&
                     !signal_interrupted(); ms += 50)
        sys_sleep_interruptible(50);
    if (l->state != DHCP_BOUND) {
        ui_error("ifup", "no DHCP answer: the interface is up without an address");
        return;
    }
    char ip[INET_ADDR_TEXT];
    inet_format(l->ip, ip);
    if (l->dns)
        dns_set_nameserver(l->dns);
    ui_ok("ifup", "configured");
    ui_printf("%s: %s/%d\n", name, ip, inet_prefix(l->netmask));
}
