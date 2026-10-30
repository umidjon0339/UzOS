/*
 * cmd_arp.c - `arp`: which MAC address answers for which IP address.
 *
 *   arp                 Address      HWtype  HWaddress          Flags  Iface
 *                       10.0.2.2     ether   52:55:0a:00:02:02  C      eth0
 *   arp -a              ? (10.0.2.2) at 52:55:0a:00:02:02 [ether] on eth0
 *   arp -s IP MAC [dev] add a permanent entry (flag M): ARP replies can
 *                       never change it (needs CAP_NET_ADMIN)
 *   arp -d IP           delete an entry
 *
 * Flags: C = complete (learned), M = permanent (by hand).
 *
 * Uses: arp_* (core/arp), netdev_* (core/netdev), inet_* / mac_format
 *       (core/inet), caps_require (security_management), arg_next,
 *       strcmp, ui_*.
 */

#include "modules/network_management/commands/arp/cmd_arp.h"        // this command
#include "modules/network_management/core/arp/arp.h"                 // arp_*
#include "modules/network_management/core/netdev/netdev.h"           // netdev_by_name, netdev_lock
#include "modules/network_management/core/inet/inet.h"               // inet_parse, inet_format, mac_format
#include "modules/security_management/core/caps/caps.h"              // caps_require, CAP_NET_ADMIN
#include "lib/args/args.h"                                           // arg_next
#include "lib/string/string.h"                                       // strcmp
#include "modules/ui/core/style/ui.h"                                // ui_*

#define USAGE "arp [-a] | arp -s IP MAC [dev] | arp -d IP"

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

static void show(int bsd_style) {
    char ip[INET_ADDR_TEXT], mac[MAC_TEXT];
    int entries = 0;
    if (!bsd_style) {
        ui_color(UI_TITLE);
        ui_printf("%-16s %-7s %-18s %-6s %s\n", "Address", "HWtype", "HWaddress", "Flags", "Iface");
        ui_reset();
    }
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        uint32_t lock = netdev_lock();          // a copy: the interrupt may change it
        arp_entry_t e = *arp_entry(i);
        netdev_unlock(lock);
        if (e.state == ARP_FREE)
            continue;
        entries++;
        inet_format(e.ip, ip);
        mac_format(e.mac, mac);
        int incomplete = e.state == ARP_INCOMPLETE;
        if (bsd_style)
            ui_printf("? (%s) at %s [ether]%s on %s\n", ip, incomplete ? "<incomplete>" : mac,
                      e.state == ARP_PERMANENT ? " PERM" : "", e.dev->name);
        else
            ui_printf("%-16s %-7s %-18s %-6s %s\n", ip, incomplete ? "" : "ether",
                      incomplete ? "(incomplete)" : mac,
                      e.state == ARP_PERMANENT ? "CM" : incomplete ? "" : "C", e.dev->name);
    }
    const arp_stats_t *s = arp_stats();
    ui_color(UI_DIM);
    ui_printf("Entries: %d   requests sent %u, received %u   replies sent %u, received %u\n",
              entries, s->requests_sent, s->requests_received, s->replies_sent, s->replies_received);
    ui_reset();
}

// arp -s IP MAC [dev]
static void add(const char *rest) {
    char ip_text[20], mac_text[20], name[NETDEV_NAME_LEN] = "eth0";
    uint32_t ip;
    uint8_t mac[6];
    rest = arg_next(rest, ip_text, sizeof(ip_text));
    rest = arg_next(rest, mac_text, sizeof(mac_text));
    arg_next(rest, name, sizeof(name));
    if (!name[0])
        strcpy(name, "eth0");               // the default interface
    netdev_t *dev = netdev_by_name(name);
    if (!inet_parse(ip_text, &ip) || !parse_mac(mac_text, mac) || !dev) {
        ui_usage(USAGE);
        return;
    }
    if (!caps_require(CAP_NET_ADMIN, "arp"))
        return;
    uint32_t lock = netdev_lock();
    int ok = arp_add_static(dev, ip, mac);
    netdev_unlock(lock);
    if (!ok)
        ui_error("arp", "the table is full");
}

// arp -d IP
static void del(const char *rest) {
    char ip_text[20];
    uint32_t ip;
    arg_next(rest, ip_text, sizeof(ip_text));
    if (!inet_parse(ip_text, &ip)) {
        ui_usage(USAGE);
        return;
    }
    if (!caps_require(CAP_NET_ADMIN, "arp"))
        return;
    uint32_t lock = netdev_lock();
    int ok = arp_delete(ip);
    netdev_unlock(lock);
    if (!ok)
        ui_error("arp", "no such entry");
}

void cmd_arp(const char *args) {
    char word[8];
    const char *rest = arg_next(args, word, sizeof(word));
    if (!word[0] || strcmp(word, "-n") == 0)
        show(0);
    else if (strcmp(word, "-a") == 0)
        show(1);
    else if (strcmp(word, "-s") == 0)
        add(rest);
    else if (strcmp(word, "-d") == 0)
        del(rest);
    else
        ui_usage(USAGE);
}
