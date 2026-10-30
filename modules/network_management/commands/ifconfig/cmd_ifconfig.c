/*
 * cmd_ifconfig.c - `ifconfig`: the classic look at the interfaces.
 *
 *   ifconfig                                every interface
 *   ifconfig eth0 10.0.2.20 netmask 255.255.255.0
 *   ifconfig eth0 down | up
 *   ifconfig eth0 mtu 576
 *   ifconfig eth0 promisc | -promisc
 *
 * Changing anything needs CAP_NET_ADMIN. On Linux, ifconfig does each
 * of these with an ioctl() on a socket (SIOCSIFADDR, SIOCSIFFLAGS, ...:
 * see syscalls/ioctl); `ip` uses a newer interface (netlink). Both end
 * in the same kernel calls, which this command makes directly.
 *
 * Uses: netdev_* (core/netdev), inet_* / mac_format (core/inet),
 *       caps_require (security_management), arg_next, atoi, strcmp, ui_*.
 */

#include "modules/network_management/commands/ifconfig/cmd_ifconfig.h"  // this command
#include "modules/network_management/core/netdev/netdev.h"          // netdev_*
#include "modules/network_management/core/inet/inet.h"              // inet_parse, inet_format, mac_format
#include "modules/security_management/core/caps/caps.h"             // caps_require, CAP_NET_ADMIN
#include "modules/process_management/core/process/process.h"        // process_caller_uid
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp
#include "modules/ui/core/style/ui.h"                               // ui_*

#define USAGE "ifconfig [dev [addr] [netmask M] [up|down] [mtu N] [promisc|-promisc]]"

// "3.4 KB": bytes in a friendlier unit, as ifconfig does.
static void size_text(uint32_t bytes) {
    if (bytes < 1000)
        ui_printf("%u B", bytes);
    else if (bytes < 1000000)
        ui_printf("%u.%u KB", bytes / 1000, (bytes % 1000) / 100);
    else
        ui_printf("%u.%u MB", bytes / 1000000, (bytes % 1000000) / 100000);
}

static void show(netdev_t *d) {
    char ip[INET_ADDR_TEXT], mask[INET_ADDR_TEXT], brd[INET_ADDR_TEXT], mac[MAC_TEXT];
    // Linux's order; RUNNING (0x40) = turned on and the cable works.
    static const struct { uint32_t bit; const char *name; } names[] = {
        { NETDEV_UP, "UP" }, { NETDEV_BROADCAST, "BROADCAST" }, { NETDEV_LOOPBACK, "LOOPBACK" },
        { 0x40, "RUNNING" }, { NETDEV_PROMISC, "PROMISC" }, { NETDEV_MULTICAST, "MULTICAST" },
    };
    uint32_t flags = d->flags | (netdev_running(d) ? 0x40 : 0);
    ui_color(UI_LINK);
    ui_printf("%s", d->name);
    ui_reset();
    ui_printf(": flags=%u<", flags);
    const char *sep = "";
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (flags & names[i].bit) {
            ui_printf("%s%s", sep, names[i].name);
            sep = ",";
        }
    }
    ui_printf(">  mtu %u\n", d->mtu);
    if (d->ip) {
        inet_format(d->ip, ip);
        inet_format(d->netmask, mask);
        inet_format(d->ip | ~d->netmask, brd);
        ui_printf("        inet ");
        ui_color(UI_ACCENT);
        ui_printf("%s", ip);
        ui_reset();
        if (d->flags & NETDEV_LOOPBACK)
            ui_printf("  netmask %s\n", mask);
        else
            ui_printf("  netmask %s  broadcast %s\n", mask, brd);
    }
    mac_format(d->mac, mac);
    if (d->flags & NETDEV_LOOPBACK)
        ui_printf("        loop  txqueuelen 1000  (Local Loopback)\n");
    else
        ui_printf("        ether %s  txqueuelen 1000  (Ethernet)\n", mac);
    const netdev_stats_t *s = &d->stats;
    ui_printf("        RX packets %u  bytes %u (", s->rx_packets, s->rx_bytes);
    size_text(s->rx_bytes);
    ui_printf(")\n        RX errors %u  dropped %u\n", s->rx_errors, s->rx_dropped);
    ui_printf("        TX packets %u  bytes %u (", s->tx_packets, s->tx_bytes);
    size_text(s->tx_bytes);
    ui_printf(")\n        TX errors %u  dropped %u\n\n", s->tx_errors, s->tx_dropped);
}

static void change(netdev_t *d, const char *rest) {
    if (!caps_require(CAP_NET_ADMIN, "ifconfig"))
        return;
    char word[20];
    uint32_t ip = d->ip, mask = d->netmask ? d->netmask : 0xFFFFFF00;
    int new_address = 0;
    uint32_t lock = netdev_lock();
    for (rest = arg_next(rest, word, sizeof(word)); word[0]; rest = arg_next(rest, word, sizeof(word))) {
        uint32_t value;
        if (strcmp(word, "up") == 0) {
            netdev_set_up(d, 1);
        } else if (strcmp(word, "down") == 0) {
            netdev_set_up(d, 0);
        } else if (strcmp(word, "promisc") == 0 || strcmp(word, "-promisc") == 0) {
            if (netdev_set_promisc(d, word[0] != '-', process_caller_uid()) < 0)
                ui_error("ifconfig", "promiscuous mode needs CAP_NET_RAW");
        } else if (strcmp(word, "mtu") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            int mtu = atoi(word);
            if (mtu >= 68 && mtu <= 1500)
                d->mtu = (uint16_t)mtu;
        } else if (strcmp(word, "netmask") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            if (inet_parse(word, &value)) {
                mask = value;
                new_address = 1;
            }
        } else if (inet_parse(word, &value)) {
            ip = value;
            new_address = 1;
        } else {
            netdev_unlock(lock);
            ui_usage(USAGE);
            return;
        }
    }
    if (new_address)
        netdev_set_address(d, ip, mask);            // also fixes the routes
    netdev_unlock(lock);
}

void cmd_ifconfig(const char *args) {
    char name[NETDEV_NAME_LEN + 4];
    const char *rest = arg_next(args, name, sizeof(name));
    if (!name[0] || strcmp(name, "-a") == 0) {
        for (int i = 0; i < netdev_count(); i++)
            show(netdev_get(i));
        return;
    }
    netdev_t *d = netdev_by_name(name);
    if (!d) {
        ui_start(UI_ERROR, "ifconfig");
        ui_printf("%s: error fetching interface information: Device not found", name);
        ui_end();
        return;
    }
    char peek[4];
    arg_next(rest, peek, sizeof(peek));
    if (peek[0])
        change(d, rest);
    else
        show(d);
}
