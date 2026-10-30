/*
 * cmd_route.c - `route`: the routing table, net-tools style.
 *
 *   route -n
 *   Kernel IP routing table
 *   Destination  Gateway   Genmask        Flags Metric Iface
 *   0.0.0.0      10.0.2.2  0.0.0.0        UG    0      eth0
 *   10.0.2.0     0.0.0.0   255.255.255.0  U     0      eth0
 *
 *   route add default gw 10.0.2.2
 *   route add -net 192.168.7.0 netmask 255.255.255.0 gw 10.0.2.2
 *   route add -host 192.168.7.9 dev eth0
 *   route del default | route del -net 192.168.7.0 netmask 255.255.255.0
 *
 * Flags: U = up, G = through a gateway, H = to one host (/32).
 * Changing the table needs CAP_NET_ADMIN.
 *
 * Uses: route_* (core/route), netdev_* (core/netdev), inet_* (core/inet),
 *       caps_require (security_management), arg_next, atoi, strcmp, ui_*.
 */

#include "modules/network_management/commands/route/cmd_route.h"    // this command
#include "modules/network_management/core/route/route.h"             // route_*
#include "modules/network_management/core/netdev/netdev.h"           // netdev_*
#include "modules/network_management/core/inet/inet.h"               // inet_parse, inet_format
#include "modules/security_management/core/caps/caps.h"              // caps_require, CAP_NET_ADMIN
#include "lib/args/args.h"                                           // arg_next
#include "lib/stdlib/stdlib.h"                                       // atoi
#include "lib/string/string.h"                                       // strcmp
#include "modules/ui/core/style/ui.h"                                // ui_*

#define USAGE "route [-n] | route add|del default|-net N netmask M|-host H [gw G] [dev D]"

static void show(void) {
    char dest[INET_ADDR_TEXT], gw[INET_ADDR_TEXT], mask[INET_ADDR_TEXT], flags[4];
    ui_printf("Kernel IP routing table\n");
    ui_color(UI_TITLE);
    ui_printf("%-15s %-15s %-15s %-5s %-6s %s\n", "Destination", "Gateway", "Genmask",
              "Flags", "Metric", "Iface");
    ui_reset();
    for (int i = 0; i < ROUTE_MAX; i++) {
        uint32_t lock = netdev_lock();
        route_t r = *route_get(i);
        netdev_unlock(lock);
        if (!r.used)
            continue;
        inet_format(r.dest, dest);
        inet_format(r.gateway, gw);
        inet_format(r.mask, mask);
        int n = 0;
        flags[n++] = 'U';
        if (r.gateway)
            flags[n++] = 'G';
        if (r.mask == 0xFFFFFFFF)
            flags[n++] = 'H';
        flags[n] = '\0';
        ui_printf("%-15s %-15s %-15s %-5s %-6u %s\n", dest, gw, mask, flags, r.metric, r.dev->name);
    }
}

// The interface whose own network contains 'ip'.
static netdev_t *dev_for(uint32_t ip) {
    for (int i = 0; i < netdev_count(); i++) {
        netdev_t *dev = netdev_get(i);
        if (dev->ip && (ip & dev->netmask) == (dev->ip & dev->netmask))
            return dev;
    }
    return 0;
}

static void change(int add, const char *rest) {
    char word[24];
    uint32_t dest = 0, mask = 0xFFFFFFFF, gateway = 0, metric = 0;
    netdev_t *dev = 0;
    int ok = 1, have_dest = 0;
    for (rest = arg_next(rest, word, sizeof(word)); word[0]; rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "default") == 0) {
            dest = mask = 0;
            have_dest = 1;
        } else if (strcmp(word, "-net") == 0 || strcmp(word, "-host") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            ok &= have_dest = inet_parse(word, &dest);
        } else if (strcmp(word, "netmask") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            ok &= inet_parse(word, &mask);
        } else if (strcmp(word, "gw") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            ok &= inet_parse(word, &gateway);
        } else if (strcmp(word, "dev") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            ok &= (dev = netdev_by_name(word)) != 0;
        } else if (strcmp(word, "metric") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            metric = (uint32_t)atoi(word);
        } else {
            ok = 0;
        }
    }
    if (!ok || !have_dest) {
        ui_usage(USAGE);
        return;
    }
    if (!caps_require(CAP_NET_ADMIN, "route"))
        return;
    dest &= mask;
    if (!add) {
        if (route_del(dest, mask) < 0)
            ui_error("route", "SIOCDELRT: No such process (no such route)");
        return;
    }
    if (!dev && gateway)
        dev = dev_for(gateway);
    if (!dev) {
        ui_error("route", gateway ? "SIOCADDRT: Network is unreachable (gw not on a link)" : "need gw or dev");
        return;
    }
    if (route_add(dest, mask, gateway, dev, metric, ROUTE_STATIC) < 0)
        ui_error("route", "SIOCADDRT: File exists (or the table is full)");
}

void cmd_route(const char *args) {
    char word[8];
    const char *rest = arg_next(args, word, sizeof(word));
    if (!word[0] || strcmp(word, "-n") == 0)
        show();
    else if (strcmp(word, "add") == 0)
        change(1, rest);
    else if (strcmp(word, "del") == 0)
        change(0, rest);
    else
        ui_usage(USAGE);
}
