/*
 * cmd_dhclient.c - `dhclient`: ask the network for an address, and watch.
 *
 *   dhclient          DHCPDISCOVER on eth0 to 255.255.255.255 port 67
 *                     DHCPOFFER of 10.0.2.15 from 10.0.2.2
 *                     DHCPREQUEST for 10.0.2.15 on eth0 to 255.255.255.255 port 67
 *                     DHCPACK of 10.0.2.15 from 10.0.2.2
 *                     bound to 10.0.2.15 -- lease 86400 seconds
 *   dhclient -r       release: give the address back to the server
 *   dhclient -l       show the lease: address, router, DNS, time left
 *
 * The conversation itself runs in the kernel (core/dhcp) as answers
 * arrive; this command starts it and prints each step as the state
 * moves on. Starting or releasing needs CAP_NET_ADMIN.
 *
 * Uses: dhcp_* (core/dhcp), dns_set_nameserver (core/dns), netdev_* (core/netdev), inet_format
 *       (core/inet), net_now_ms (core/net_tick), caps_require
 *       (security_management), sys_sleep_interruptible / signal_interrupted
 *       (process_management), arg_next, strcmp, ui_*.
 */

#include "modules/network_management/commands/dhclient/cmd_dhclient.h"  // this command
#include "modules/network_management/core/dhcp/dhcp.h"               // dhcp_*
#include "modules/network_management/core/dns/dns.h"                 // dns_set_nameserver
#include "modules/network_management/core/netdev/netdev.h"           // netdev_by_name, netdev_lock
#include "modules/network_management/core/inet/inet.h"               // inet_format, inet_prefix
#include "modules/network_management/core/net_tick/net_tick.h"       // net_now_ms
#include "modules/security_management/core/caps/caps.h"              // caps_require, CAP_NET_ADMIN
#include "modules/process_management/syscalls/sleep/sys_sleep.h"     // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"           // signal_interrupted
#include "lib/args/args.h"                                           // arg_next
#include "lib/string/string.h"                                       // strcmp
#include "modules/ui/core/style/ui.h"                                // ui_*

#define USAGE    "dhclient [-r | -l] [eth0]"
#define STEP_MS  10
#define GIVE_UP_MS 12000        // a little more than all the retries

static void show_lease(void) {
    const dhcp_lease_t *l = dhcp_lease();
    char ip[INET_ADDR_TEXT], router[INET_ADDR_TEXT], dns[INET_ADDR_TEXT], server[INET_ADDR_TEXT];
    ui_printf("state:   %s\n", dhcp_state_name(l->state));
    if (l->state != DHCP_BOUND)
        return;
    inet_format(l->ip, ip);
    inet_format(l->router, router);
    inet_format(l->dns, dns);
    inet_format(l->server, server);
    uint32_t used_s = (net_now_ms() - l->bound_ms) / 1000;
    ui_printf("address: %s/%d on %s\n", ip, inet_prefix(l->netmask), l->dev->name);
    ui_printf("router:  %s\n", router);
    ui_printf("dns:     %s\n", dns);
    ui_printf("server:  %s\n", server);
    ui_printf("lease:   %u seconds, %u left\n", l->lease_s,
              l->lease_s > used_s ? l->lease_s - used_s : 0);
}

// One line per message, as ISC dhclient prints them.
static void print_event(const dhcp_event_t *e, netdev_t *dev) {
    char ip[INET_ADDR_TEXT], server[INET_ADDR_TEXT];
    inet_format(e->ip, ip);
    inet_format(e->server, server);
    switch (e->type) {
    case DHCPDISCOVER: ui_printf("DHCPDISCOVER on %s to 255.255.255.255 port 67\n", dev->name); break;
    case DHCPOFFER:    ui_printf("DHCPOFFER of %s from %s\n", ip, server); break;
    case DHCPREQUEST:  ui_printf("DHCPREQUEST for %s on %s to 255.255.255.255 port 67\n", ip, dev->name); break;
    case DHCPACK:      ui_printf("DHCPACK of %s from %s\n", ip, server); break;
    case DHCPNAK:      ui_printf("DHCPNAK from %s\n", server); break;
    }
}

// Start, then print every message until BOUND or FAILED. The exchange
// may be over before we first look (QEMU answers at once), so the
// messages come from the core's log, not from watching the state.
static void run(netdev_t *dev) {
    int next = dhcp_log_count();
    uint32_t lock = netdev_lock();
    dhcp_start(dev);
    netdev_unlock(lock);

    const dhcp_lease_t *l = dhcp_lease();
    for (int waited = 0; waited < GIVE_UP_MS && !signal_interrupted(); waited += STEP_MS) {
        for (const dhcp_event_t *e; next < dhcp_log_count() && (e = dhcp_log_get(next)); next++)
            print_event(e, dev);
        if (l->state == DHCP_BOUND) {
            char ip[INET_ADDR_TEXT];
            inet_format(l->ip, ip);
            ui_color(UI_GOOD);
            ui_printf("bound to %s -- lease %u seconds\n", ip, l->lease_s);
            ui_reset();
            if (l->dns)
                dns_set_nameserver(l->dns);         // /etc/resolv.conf
            return;
        }
        if (l->state == DHCP_FAILED) {
            ui_error("dhclient", "No DHCPOFFERS received (or the server said NAK)");
            return;
        }
        sys_sleep_interruptible(STEP_MS);
    }
    ui_warn("dhclient", "gave up waiting");
}

void cmd_dhclient(const char *args) {
    char word[NETDEV_NAME_LEN] = "", option = 0;
    char name[NETDEV_NAME_LEN] = "eth0";
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (word[0] == '-' && word[1] && !word[2])
            option = word[1];
        else
            strcpy(name, word);
    }
    netdev_t *dev = netdev_by_name(name);
    if (!dev || (dev->flags & NETDEV_LOOPBACK) || (option && option != 'r' && option != 'l' && option != 'v')) {
        ui_usage(USAGE);
        return;
    }
    if (option == 'l') {
        show_lease();
        return;
    }
    if (!caps_require(CAP_NET_ADMIN, "dhclient"))
        return;
    if (option == 'r') {
        uint32_t lock = netdev_lock();
        dhcp_release();
        netdev_unlock(lock);
        ui_printf("DHCPRELEASE: %s has no address now\n", dev->name);
        return;
    }
    if (!netdev_running(dev)) {
        ui_error("dhclient", "the interface is down (ip link set eth0 up)");
        return;
    }
    run(dev);
}
