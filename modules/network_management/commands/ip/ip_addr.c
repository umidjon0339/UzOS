/*
 * ip_addr.c - `ip addr`: the IPv4 address of each interface (layer 3).
 *
 *   ip addr                              every interface and its address
 *   ip addr show eth0
 *   ip addr add 10.0.2.15/24 dev eth0    set it (needs CAP_NET_ADMIN)
 *   ip addr del 10.0.2.15/24 dev eth0    remove it
 *   ip addr flush dev eth0               the same, without naming it
 *
 * "/24" is the PREFIX: the first 24 bits name the network (10.0.2.x),
 * the rest the machine. Its mask is 255.255.255.0; the broadcast
 * address (everyone on the network) is the network with all
 * machine bits set: 10.0.2.255.
 *
 * In UzOS an interface has one IPv4 address (Linux allows several).
 *
 * Uses: netdev_* (core/netdev), inet_* (core/inet), ip_link_print
 *       (ip_link.c), caps_require (security_management), arg_next, atoi,
 *       strcmp, strcpy, ui_*.
 */

#include "modules/network_management/commands/ip/cmd_ip.h"           // ip_addr, ip_link_print
#include "modules/network_management/core/netdev/netdev.h"           // netdev_*
#include "modules/network_management/core/inet/inet.h"               // inet_parse, inet_format, inet_mask
#include "modules/security_management/core/caps/caps.h"              // caps_require, CAP_NET_ADMIN
#include "lib/args/args.h"                                           // arg_next
#include "lib/stdlib/stdlib.h"                                       // atoi
#include "lib/string/string.h"                                       // strcmp
#include "modules/ui/core/style/ui.h"                                // ui_*

#define USAGE "ip addr [show [dev] | add|del A.B.C.D/N dev D | flush dev D]"

static void print_addr(netdev_t *dev) {
    ip_link_print(dev, 0);
    if (!dev->ip)
        return;
    char ip[INET_ADDR_TEXT], brd[INET_ADDR_TEXT];
    inet_format(dev->ip, ip);
    ui_printf("    inet ");
    ui_color(UI_ACCENT);
    ui_printf("%s/%d", ip, inet_prefix(dev->netmask));
    ui_reset();
    if (dev->flags & NETDEV_LOOPBACK) {
        ui_printf(" scope host %s\n", dev->name);
    } else {
        inet_format(dev->ip | ~dev->netmask, brd);
        ui_printf(" brd %s scope global %s\n", brd, dev->name);
    }
}

// "10.0.2.15/24" -> ip, mask. No "/N" means /32 (one machine), as in Linux.
static int parse_cidr(char *text, uint32_t *ip, uint32_t *mask) {
    int prefix = 32;
    for (char *c = text; *c; c++) {
        if (*c == '/') {
            *c = '\0';
            prefix = atoi(c + 1);
            if (prefix < 0 || prefix > 32 || !c[1])
                return 0;
            break;
        }
    }
    *mask = inet_mask(prefix);
    return inet_parse(text, ip);
}

// add | del A.B.C.D/N dev D        flush dev D
static void change(const char *verb, const char *rest) {
    char word[24], cidr[24] = "", name[NETDEV_NAME_LEN] = "";
    for (rest = arg_next(rest, word, sizeof(word)); word[0]; rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "dev") == 0)
            rest = arg_next(rest, name, sizeof(name));
        else
            strcpy(cidr, word);
    }
    netdev_t *dev = netdev_by_name(name);
    uint32_t ip = 0, mask = 0;
    int flush = strcmp(verb, "flush") == 0;
    if (!dev || (!flush && !parse_cidr(cidr, &ip, &mask))) {
        ui_usage(USAGE);
        return;
    }
    if (!caps_require(CAP_NET_ADMIN, "ip addr"))
        return;

    if (strcmp(verb, "add") == 0) {
        netdev_set_address(dev, ip, mask);
    } else if (flush || ip == dev->ip) {
        netdev_set_address(dev, 0, 0);
    } else {
        ui_error("ip", "that is not the address of this interface");
    }
}

void ip_addr(const char *rest) {
    char word[NETDEV_NAME_LEN + 4];
    const char *after = arg_next(rest, word, sizeof(word));

    if (!strcmp(word, "add") || !strcmp(word, "del") || !strcmp(word, "flush")) {
        change(word, after);
        return;
    }
    if (strcmp(word, "show") == 0)
        after = arg_next(after, word, sizeof(word));
    if (strcmp(word, "dev") == 0)
        arg_next(after, word, sizeof(word));

    if (word[0]) {
        netdev_t *dev = netdev_by_name(word);
        if (dev)
            print_addr(dev);
        else
            ui_error("ip", "no such interface");
        return;
    }
    for (int i = 0; i < netdev_count(); i++)
        print_addr(netdev_get(i));
}
