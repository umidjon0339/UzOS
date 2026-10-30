/*
 * ip_route.c - `ip route`: the routing table.
 *
 *   ip route                            default via 10.0.2.2 dev eth0 proto static
 *                                       10.0.2.0/24 dev eth0 proto kernel scope link src 10.0.2.15
 *   ip route add default via 10.0.2.2   (needs CAP_NET_ADMIN)
 *   ip route add 192.168.7.0/24 via 10.0.2.2 [dev eth0] [metric N]
 *   ip route add 192.168.7.0/24 dev eth0
 *   ip route del default | 192.168.7.0/24
 *   ip route get 8.8.8.8                which route a packet would take
 *
 * "via" names a gateway (a router on our link); without one the
 * destination is on the link itself ("scope link").
 *
 * Uses: route_* (core/route), netdev_* (core/netdev), inet_* (core/inet),
 *       caps_require (security_management), arg_next, atoi, strcmp, ui_*.
 */

#include "modules/network_management/commands/ip/cmd_ip.h"           // ip_route
#include "modules/network_management/core/route/route.h"             // route_*
#include "modules/network_management/core/netdev/netdev.h"           // netdev_by_name, netdev_get
#include "modules/network_management/core/inet/inet.h"               // inet_parse, inet_format, inet_mask
#include "modules/security_management/core/caps/caps.h"              // caps_require, CAP_NET_ADMIN
#include "lib/args/args.h"                                           // arg_next
#include "lib/stdlib/stdlib.h"                                       // atoi
#include "lib/string/string.h"                                       // strcmp
#include "modules/ui/core/style/ui.h"                                // ui_*

#define USAGE "ip route [add|del NET/N|default [via GW] [dev D] [metric M] | get IP]"

static const char *proto_name(route_proto_t proto) {
    return proto == ROUTE_KERNEL ? "kernel" : proto == ROUTE_DHCP ? "dhcp" : "static";
}

static void show(void) {
    char dest[INET_ADDR_TEXT], gw[INET_ADDR_TEXT], src[INET_ADDR_TEXT];
    for (int i = 0; i < ROUTE_MAX; i++) {
        uint32_t lock = netdev_lock();          // a copy: it may change meanwhile
        route_t r = *route_get(i);
        netdev_unlock(lock);
        if (!r.used)
            continue;
        if (r.mask == 0) {
            ui_color(UI_ACCENT);
            ui_printf("default");
            ui_reset();
        } else {
            inet_format(r.dest, dest);
            ui_printf("%s/%d", dest, inet_prefix(r.mask));
        }
        if (r.gateway) {
            inet_format(r.gateway, gw);
            ui_printf(" via %s", gw);
        }
        ui_printf(" dev %s proto %s", r.dev->name, proto_name(r.proto));
        if (!r.gateway) {
            inet_format(r.dev->ip, src);
            ui_printf(" scope link src %s", src);
        }
        if (r.metric)
            ui_printf(" metric %u", r.metric);
        ui_printf("\n");
    }
}

// The interface whose own network contains 'ip' (for "via" without "dev").
static netdev_t *dev_for(uint32_t ip) {
    for (int i = 0; i < netdev_count(); i++) {
        netdev_t *dev = netdev_get(i);
        if (dev->ip && (ip & dev->netmask) == (dev->ip & dev->netmask))
            return dev;
    }
    return 0;
}

// "default" or "A.B.C.D/N" (no /N = one host, /32).
static int parse_dest(char *text, uint32_t *dest, uint32_t *mask) {
    if (strcmp(text, "default") == 0) {
        *dest = *mask = 0;
        return 1;
    }
    int prefix = 32;
    for (char *c = text; *c; c++) {
        if (*c == '/') {
            *c = '\0';
            prefix = atoi(c + 1);
            break;
        }
    }
    if (prefix < 0 || prefix > 32 || !inet_parse(text, dest))
        return 0;
    *mask = inet_mask(prefix);
    *dest &= *mask;
    return 1;
}

static void change(int add, const char *rest) {
    char word[24], target[24] = "";
    uint32_t dest, mask, gateway = 0, metric = 0;
    netdev_t *dev = 0;
    int ok = 1;
    for (rest = arg_next(rest, word, sizeof(word)); word[0]; rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "via") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            ok &= inet_parse(word, &gateway);
        } else if (strcmp(word, "dev") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            ok &= (dev = netdev_by_name(word)) != 0;
        } else if (strcmp(word, "metric") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            metric = (uint32_t)atoi(word);
        } else {
            strcpy(target, word);
        }
    }
    if (!ok || !parse_dest(target, &dest, &mask)) {
        ui_usage(USAGE);
        return;
    }
    if (!caps_require(CAP_NET_ADMIN, "ip route"))
        return;

    if (!add) {
        if (route_del(dest, mask) < 0)
            ui_error("ip", "RTNETLINK answers: No such process (no such route)");
        return;
    }
    if (!dev && gateway)
        dev = dev_for(gateway);
    if (!dev) {
        ui_error("ip", gateway ? "Nexthop has invalid gateway (not on any link)" : "need via or dev");
        return;
    }
    int err = route_add(dest, mask, gateway, dev, metric, ROUTE_STATIC);
    if (err == -NET_EADDRINUSE)
        ui_error("ip", "RTNETLINK answers: File exists");
    else if (err < 0)
        ui_error("ip", "the routing table is full");
}

// ip route get IP: what route_lookup() decides.
static void get(const char *rest) {
    char word[24], text[INET_ADDR_TEXT];
    uint32_t ip;
    arg_next(rest, word, sizeof(word));
    if (!inet_parse(word, &ip)) {
        ui_usage("ip route get IP");
        return;
    }
    route_result_t r;
    uint32_t lock = netdev_lock();
    int found = route_lookup(ip, &r);
    netdev_unlock(lock);
    if (!found) {
        ui_error("ip", "RTNETLINK answers: Network is unreachable");
        return;
    }
    ui_printf("%s", word);
    if (r.next_hop != ip) {
        inet_format(r.next_hop, text);
        ui_printf(" via %s", text);
    }
    inet_format(r.src, text);
    ui_printf(" dev %s src %s\n", r.dev->name, text);
}

void ip_route(const char *rest) {
    char word[8];
    const char *after = arg_next(rest, word, sizeof(word));
    if (!word[0] || strcmp(word, "show") == 0 || strcmp(word, "list") == 0)
        show();
    else if (strcmp(word, "add") == 0)
        change(1, after);
    else if (strcmp(word, "del") == 0)
        change(0, after);
    else if (strcmp(word, "get") == 0)
        get(after);
    else
        ui_usage(USAGE);
}
