/*
 * ip_neigh.c - `ip neigh`: the neighbours (ARP cache), IP -> MAC.
 *
 *   ip neigh                                       10.0.2.2 dev eth0 lladdr 52:55:0a:00:02:02 REACHABLE
 *   ip neigh add 10.0.2.9 lladdr 52:54:00:00:00:09 dev eth0    a PERMANENT entry
 *   ip neigh del 10.0.2.9 dev eth0
 *   ip neigh flush dev eth0                        forget what was learned
 *
 * Changing the table needs CAP_NET_ADMIN.
 *
 * Uses: arp_* (core/arp), netdev_by_name (core/netdev), inet_* / mac_format
 *       (core/inet), caps_require (security_management), netdev_lock,
 *       arg_next, strcmp, ui_*.
 */

#include "modules/network_management/commands/ip/cmd_ip.h"           // ip_neigh
#include "modules/network_management/core/arp/arp.h"                 // arp_*
#include "modules/network_management/core/netdev/netdev.h"           // netdev_by_name, netdev_lock
#include "modules/network_management/core/inet/inet.h"               // inet_parse, inet_format, mac_format
#include "modules/security_management/core/caps/caps.h"              // caps_require, CAP_NET_ADMIN
#include "lib/args/args.h"                                           // arg_next
#include "lib/string/string.h"                                       // strcmp
#include "modules/ui/core/style/ui.h"                                // ui_*

#define USAGE "ip neigh [add IP lladdr MAC dev D | del IP dev D | flush dev D]"

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

static const char *state_name(arp_state_t state) {
    switch (state) {
    case ARP_INCOMPLETE: return "INCOMPLETE";
    case ARP_REACHABLE:  return "REACHABLE";
    case ARP_PERMANENT:  return "PERMANENT";
    default:             return "?";
    }
}

static void show(void) {
    char ip[INET_ADDR_TEXT], mac[MAC_TEXT];
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        // A copy: the card's interrupt may change the entry while we print.
        uint32_t lock = netdev_lock();
        arp_entry_t copy = *arp_entry(i);
        netdev_unlock(lock);
        const arp_entry_t *e = &copy;
        if (e->state == ARP_FREE)
            continue;
        inet_format(e->ip, ip);
        ui_printf("%s dev %s ", ip, e->dev->name);
        if (e->state != ARP_INCOMPLETE) {
            mac_format(e->mac, mac);
            ui_printf("lladdr %s ", mac);
        }
        ui_color(e->state == ARP_INCOMPLETE ? UI_WARNING : UI_GOOD);
        ui_printf("%s\n", state_name(e->state));
        ui_reset();
    }
}

static void change(const char *verb, const char *rest) {
    char word[20], name[NETDEV_NAME_LEN] = "";
    uint32_t ip = 0;
    uint8_t mac[6];
    int have_ip = 0, have_mac = 0;
    for (rest = arg_next(rest, word, sizeof(word)); word[0]; rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "dev") == 0) {
            rest = arg_next(rest, name, sizeof(name));
        } else if (strcmp(word, "lladdr") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            have_mac = parse_mac(word, mac);
        } else {
            have_ip = inet_parse(word, &ip);
        }
    }
    netdev_t *dev = netdev_by_name(name);
    int add = strcmp(verb, "add") == 0, flush = strcmp(verb, "flush") == 0;
    if (!dev || (!flush && !have_ip) || (add && !have_mac)) {
        ui_usage(USAGE);
        return;
    }
    if (!caps_require(CAP_NET_ADMIN, "ip neigh"))
        return;

    uint32_t lock = netdev_lock();
    int ok = 1;
    if (add)
        ok = arp_add_static(dev, ip, mac);
    else if (flush)
        arp_flush(dev);
    else
        ok = arp_delete(ip);
    netdev_unlock(lock);
    if (!ok)
        ui_error("ip", add ? "the table is full" : "no such entry");
}

void ip_neigh(const char *rest) {
    char word[8];
    const char *after = arg_next(rest, word, sizeof(word));
    if (!strcmp(word, "add") || !strcmp(word, "del") || !strcmp(word, "flush"))
        change(word, after);
    else if (!word[0] || !strcmp(word, "show"))
        show();
    else
        ui_usage(USAGE);
}
