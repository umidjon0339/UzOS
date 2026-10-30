/*
 * cmd_ip.c - `ip [-s] <object> ...`: pick the object, the rest is its file.
 *
 *   ip link   (ip l)    the interfaces: lo, eth0            ip_link.c
 *   ip addr   (ip a)    their IPv4 addresses                ip_addr.c
 *   ip neigh  (ip n)    the ARP cache: IP -> MAC            ip_neigh.c
 *   ip route  (ip r)    the routing table                   ip_route.c
 *
 * As in Linux, an object may be shortened to its first letter.
 *
 * Uses: ip_* (this folder), arg_next (lib/args), strcmp (lib/string), ui_usage.
 */

#include "modules/network_management/commands/ip/cmd_ip.h"   // this command
#include "lib/args/args.h"                                    // arg_next
#include "lib/string/string.h"                                // strcmp
#include "modules/ui/core/style/ui.h"                         // ui_usage

#define USAGE "ip [-s] link|addr|route|neigh ... (try: ip link, ip addr, ip route)"

// "l" or "link" both mean link.
static int is(const char *word, const char *object) {
    return strcmp(word, object) == 0 || (word[0] == object[0] && !word[1]);
}

void cmd_ip(const char *args) {
    char word[16];
    int stats = 0;
    const char *rest = arg_next(args, word, sizeof(word));
    if (strcmp(word, "-s") == 0) {
        stats = 1;
        rest = arg_next(rest, word, sizeof(word));
    }

    if (is(word, "link"))
        ip_link(rest, stats);
    else if (is(word, "addr") || strcmp(word, "address") == 0)
        ip_addr(rest);
    else if (is(word, "route"))
        ip_route(rest);
    else if (is(word, "neigh") || strcmp(word, "neighbour") == 0)
        ip_neigh(rest);
    else
        ui_usage(USAGE);
}
