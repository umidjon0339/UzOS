/*
 * ip_link.c - `ip link`: the interfaces (layer 2: MAC, MTU, up or down).
 *
 *   ip link                       1: lo: <LOOPBACK,UP,LOWER_UP> mtu 1500 state UNKNOWN
 *                                     link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
 *   ip -s link                    ...and RX / TX counters
 *   ip link show eth0             one interface
 *   ip link set eth0 down         turn it off (needs CAP_NET_ADMIN)
 *   ip link set eth0 promisc on   receive every frame on the wire
 *   ip link set eth0 mtu 1400
 *   ip link set eth0 address 02:00:00:00:00:01   a new MAC (mac_lock)
 *
 * UP is "turned on"; LOWER_UP is "the cable works" (carrier).
 *
 * Uses: netdev_* (core/netdev), mac_format (core/inet), caps_require
 *       (security_management), arg_next, atoi, strcmp, ui_*.
 */

#include "modules/network_management/commands/ip/cmd_ip.h"           // ip_link
#include "modules/network_management/core/netdev/netdev.h"           // netdev_*
#include "modules/network_management/core/inet/inet.h"               // mac_format
#include "modules/security_management/core/caps/caps.h"              // caps_require, CAP_NET_ADMIN
#include "modules/process_management/core/process/process.h"         // process_caller_uid
#include "lib/args/args.h"                                           // arg_next
#include "lib/stdlib/stdlib.h"                                       // atoi
#include "lib/string/string.h"                                       // strcmp
#include "modules/ui/core/style/ui.h"                                // ui_*

#define MTU_MIN 68          // the smallest MTU IPv4 allows
#define MTU_MAX 1500        // our packet buffers hold one Ethernet frame

// <LOOPBACK,UP,LOWER_UP>
static void print_flags(netdev_t *dev) {
    static const struct { uint32_t bit; const char *name; } names[] = {
        { NETDEV_LOOPBACK, "LOOPBACK" }, { NETDEV_BROADCAST, "BROADCAST" },
        { NETDEV_MULTICAST, "MULTICAST" }, { NETDEV_PROMISC, "PROMISC" },
        { NETDEV_UP, "UP" },
    };
    const char *sep = "";
    ui_printf("<");
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (dev->flags & names[i].bit) {
            ui_printf("%s%s", sep, names[i].name);
            sep = ",";
        }
    }
    if (netdev_running(dev))
        ui_printf("%sLOWER_UP", sep);
    ui_printf(">");
}

static const char *state(netdev_t *dev) {
    if (dev->flags & NETDEV_LOOPBACK)
        return "UNKNOWN";               // lo has no carrier to report (as in Linux)
    return netdev_running(dev) ? "UP" : "DOWN";
}

void ip_link_print(netdev_t *dev, int stats) {
    char mac[MAC_TEXT];
    ui_printf("%d: ", dev->index);
    ui_color(UI_LINK);
    ui_printf("%s", dev->name);
    ui_reset();
    ui_printf(": ");
    print_flags(dev);
    ui_printf(" mtu %u state ", dev->mtu);
    ui_color(netdev_running(dev) || (dev->flags & NETDEV_LOOPBACK) ? UI_GOOD : UI_BAD);
    ui_printf("%s\n", state(dev));
    ui_reset();

    mac_format(dev->mac, mac);
    if (dev->flags & NETDEV_LOOPBACK)
        ui_printf("    link/loopback %s brd 00:00:00:00:00:00\n", mac);
    else
        ui_printf("    link/ether %s brd ff:ff:ff:ff:ff:ff\n", mac);

    if (stats) {
        const netdev_stats_t *s = &dev->stats;
        ui_color(UI_DIM);
        ui_printf("    RX:  %10s %8s %7s %7s\n", "bytes", "packets", "errors", "dropped");
        ui_reset();
        ui_printf("         %10u %8u %7u %7u\n", s->rx_bytes, s->rx_packets, s->rx_errors, s->rx_dropped);
        ui_color(UI_DIM);
        ui_printf("    TX:  %10s %8s %7s %7s\n", "bytes", "packets", "errors", "dropped");
        ui_reset();
        ui_printf("         %10u %8u %7u %7u\n", s->tx_bytes, s->tx_packets, s->tx_errors, s->tx_dropped);
    }
}

// "52:54:00:12:34:56" -> 6 bytes. Returns 1 if it is a MAC.
static int parse_mac(const char *text, uint8_t mac[6]) {
    for (int i = 0; i < 6; i++) {
        int value = 0;
        for (int digit = 0; digit < 2; digit++, text++) {
            char c = *text;
            int v = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 :
                    c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
            if (v < 0)
                return 0;
            value = value * 16 + v;
        }
        mac[i] = (uint8_t)value;
        if (i < 5 && *text++ != ':')
            return 0;
    }
    return *text == '\0';
}

// ip link set <dev> up | down | promisc on|off | mtu N | address MAC
static void set(const char *rest) {
    char name[NETDEV_NAME_LEN], what[12], value[20];
    rest = arg_next(rest, name, sizeof(name));
    rest = arg_next(rest, what, sizeof(what));
    arg_next(rest, value, sizeof(value));

    netdev_t *dev = netdev_by_name(name);
    if (!dev) {
        ui_error("ip", "no such interface");
        return;
    }
    if (!caps_require(CAP_NET_ADMIN, "ip link set"))
        return;

    uint32_t lock = netdev_lock();
    if (strcmp(what, "up") == 0) {
        netdev_set_up(dev, 1);
    } else if (strcmp(what, "down") == 0) {
        netdev_set_up(dev, 0);
    } else if (strcmp(what, "promisc") == 0 && (!strcmp(value, "on") || !strcmp(value, "off"))) {
        if (netdev_set_promisc(dev, strcmp(value, "on") == 0, process_caller_uid()) < 0)
            ui_error("ip", "promiscuous mode needs CAP_NET_RAW");
    } else if (strcmp(what, "address") == 0) {
        uint8_t mac[6];
        if (!parse_mac(value, mac) || netdev_set_mac(dev, mac, process_caller_uid()) < 0)
            ui_error("ip", "a MAC like 02:00:00:00:00:01, and CAP_NET_ADMIN");
    } else if (strcmp(what, "mtu") == 0 && atoi(value) >= MTU_MIN && atoi(value) <= MTU_MAX) {
        dev->mtu = (uint16_t)atoi(value);
    } else {
        netdev_unlock(lock);
        ui_usage("ip link set dev up|down|promisc on|off|mtu 68-1500|address MAC");
        return;
    }
    netdev_unlock(lock);
}

void ip_link(const char *rest, int stats) {
    char word[NETDEV_NAME_LEN + 4];
    rest = arg_next(rest, word, sizeof(word));

    if (strcmp(word, "set") == 0) {
        set(rest);
        return;
    }
    if (strcmp(word, "show") == 0)                  // "show" is the default
        rest = arg_next(rest, word, sizeof(word));
    if (strcmp(word, "dev") == 0)
        arg_next(rest, word, sizeof(word));

    if (word[0]) {
        netdev_t *dev = netdev_by_name(word);
        if (dev)
            ip_link_print(dev, stats);
        else
            ui_error("ip", "no such interface");
        return;
    }
    for (int i = 0; i < netdev_count(); i++)
        ip_link_print(netdev_get(i), stats);
}
