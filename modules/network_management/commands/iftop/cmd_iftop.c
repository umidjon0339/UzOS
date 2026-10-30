/*
 * cmd_iftop.c - `iftop`: the busiest conversations, every 2 seconds.
 *
 *   iftop -t 10
 *   --- last 2 s ------------------------------------------
 *   10.0.2.15           => 93.184.215.14          1.2 KB/s
 *   93.184.215.14       => 10.0.2.15             12.4 KB/s
 *
 * A tap (like tcpdump's) adds each IPv4 packet's size to its (from, to)
 * pair; every INTERVAL_MS the loop prints the pairs, fastest first, and
 * starts counting again. Seeing everyone's traffic needs CAP_NET_RAW.
 *
 * Uses: netdev_set_tap / netdev_by_name (core/netdev), ETH_* / ipv4_header_t
 *       (core), inet_format (core/inet), caps_require (security_management),
 *       sys_sleep_interruptible / signal_interrupted, arg_next, atoi, ui_*.
 */

#include "modules/network_management/commands/iftop/cmd_iftop.h"    // this command
#include "modules/network_management/core/netdev/netdev.h"          // netdev_set_tap, netdev_by_name
#include "modules/network_management/core/ethernet/ethernet.h"      // ETH_HLEN, ETH_P_IP
#include "modules/network_management/core/ipv4/ipv4.h"              // ipv4_header_t
#include "modules/network_management/core/inet/inet.h"              // inet_format, ntohl
#include "modules/security_management/core/caps/caps.h"             // caps_require, CAP_NET_RAW
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"          // signal_interrupted
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp, memset
#include "modules/ui/core/style/ui.h"                               // ui_*

#define FLOWS       16
#define INTERVAL_MS 2000
#define SHOWN       8

typedef struct {
    uint32_t from, to;
    uint32_t bytes;
} flow_t;

static flow_t flows[FLOWS];
static netdev_t *watched;

// In the interrupt: add this packet's size to its pair.
static void count(netdev_t *dev, const uint8_t *frame, uint16_t len, int outgoing) {
    (void)outgoing;
    if ((watched && dev != watched) || len < ETH_HLEN + IPV4_HLEN || ((frame[12] << 8) | frame[13]) != ETH_P_IP)
        return;
    const ipv4_header_t *ip = (const ipv4_header_t *)(frame + ETH_HLEN);
    uint32_t from = ntohl(ip->src), to = ntohl(ip->dst);
    flow_t *spot = 0;
    for (int i = 0; i < FLOWS && !spot; i++)
        if (flows[i].bytes && flows[i].from == from && flows[i].to == to)
            spot = &flows[i];
    for (int i = 0; i < FLOWS && !spot; i++)
        if (!flows[i].bytes)
            spot = &flows[i];
    if (!spot)
        return;                                 // more pairs than we count: the rest wait
    spot->from = from;
    spot->to = to;
    spot->bytes += len;
}

static void rate(uint32_t bytes) {
    uint32_t per_s = bytes * 1000 / INTERVAL_MS;
    if (per_s < 1024)
        ui_printf("%6u  B/s", per_s);
    else
        ui_printf("%4u.%u KB/s", per_s / 1024, (per_s % 1024) * 10 / 1024);
}

void cmd_iftop(const char *args) {
    char word[NETDEV_NAME_LEN + 4], name[NETDEV_NAME_LEN] = "eth0";
    int seconds = 0;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-i") == 0)
            rest = arg_next(rest, name, sizeof(name));
        else if (strcmp(word, "-t") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            seconds = atoi(word);
        }
    }
    watched = strcmp(name, "any") == 0 ? 0 : netdev_by_name(name);
    if (strcmp(name, "any") != 0 && !watched) {
        ui_error("iftop", "no such interface");
        return;
    }
    if (!caps_require(CAP_NET_RAW, "iftop"))
        return;

    memset(flows, 0, sizeof(flows));
    uint32_t lock = netdev_lock();
    netdev_set_tap(count);
    netdev_unlock(lock);
    ui_printf("iftop: interface %s, every %d s, Ctrl+C to stop\n", name, INTERVAL_MS / 1000);

    for (int elapsed = 0; !signal_interrupted() && (!seconds || elapsed < seconds * 1000); elapsed += INTERVAL_MS) {
        for (int w = 0; w < INTERVAL_MS && !signal_interrupted(); w += 50)
            sys_sleep_interruptible(50);
        flow_t snap[FLOWS];
        lock = netdev_lock();
        for (int i = 0; i < FLOWS; i++)
            snap[i] = flows[i];
        memset(flows, 0, sizeof(flows));        // count the next interval afresh
        netdev_unlock(lock);

        ui_color(UI_DIM);
        ui_printf("--- last %d s ---------------------------------------------\n", INTERVAL_MS / 1000);
        ui_reset();
        for (int shown = 0; shown < SHOWN; shown++) {
            int best = -1;                      // the busiest pair not printed yet
            for (int i = 0; i < FLOWS; i++)
                if (snap[i].bytes && (best < 0 || snap[i].bytes > snap[best].bytes))
                    best = i;
            if (best < 0)
                break;
            char a[INET_ADDR_TEXT], b[INET_ADDR_TEXT];
            inet_format(snap[best].from, a);
            inet_format(snap[best].to, b);
            ui_printf("%-16s => %-16s ", a, b);
            ui_color(UI_ACCENT);
            rate(snap[best].bytes);
            ui_reset();
            ui_printf("\n");
            snap[best].bytes = 0;
        }
    }
    lock = netdev_lock();
    netdev_set_tap(0);
    netdev_unlock(lock);
}
