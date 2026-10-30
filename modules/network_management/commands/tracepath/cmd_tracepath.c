/*
 * cmd_tracepath.c - `tracepath host`: the hops, and the path's MTU.
 *
 *   tracepath example.com
 *    1?: [LOCALHOST]                      pmtu 1500
 *    1:  10.0.2.2                          0.512ms reached
 *        Resume: pmtu 1500 hops 1 back 1
 *
 * Like traceroute (one probe per TTL, each router answers "time
 * exceeded"), plus the "path MTU": the biggest packet that passes every
 * link. It starts at our interface's MTU; a router with a smaller link
 * would answer "fragmentation needed" with its MTU (none does under
 * QEMU's user network).
 *
 * Uses: icmp_send_echo / icmp_set_observer (core/icmp), route_lookup
 *       (core/route), dns_resolve (core/dns), inet_format (core/inet),
 *       tsc_now (cpu_management), sys_sleep_interruptible /
 *       signal_interrupted (process_management), arg_next, atoi, strcmp, ui_*.
 */

#include "modules/network_management/commands/tracepath/cmd_tracepath.h"  // this command
#include "modules/network_management/core/icmp/icmp.h"              // icmp_send_echo, icmp_set_observer
#include "modules/network_management/core/route/route.h"            // route_lookup
#include "modules/network_management/core/netdev/netdev.h"          // netdev_lock
#include "modules/network_management/core/dns/dns.h"                // dns_resolve
#include "modules/network_management/core/inet/inet.h"              // inet_format
#include "modules/cpu_management/core/tsc/tsc.h"                    // tsc_now, tsc_cycles_to_us
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"          // signal_interrupted
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp, strcpy
#include "modules/ui/core/style/ui.h"                               // ui_*

#define WAIT_MS 1000
#define PROBE_ID 0x7470             // "tp"

static volatile int answered;
static volatile uint8_t answer_type;
static volatile uint32_t answer_from;
static volatile uint64_t answer_tsc;
static uint16_t probe_seq;

static void on_icmp(const icmp_event_t *e) {
    if (answered || e->id != PROBE_ID || e->seq != probe_seq)
        return;
    answer_type = e->type;
    answer_from = e->from;
    answer_tsc = tsc_now();
    answered = 1;
}

void cmd_tracepath(const char *args) {
    char word[DNS_NAME_MAX], host[DNS_NAME_MAX] = "";
    int max_hops = 30;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-m") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            max_hops = atoi(word);
        } else {
            strcpy(host, word);
        }
    }
    uint32_t dst;
    if (!host[0] || max_hops < 1 || max_hops > 64 || dns_resolve(host, &dst) != DNS_OK) {
        ui_usage("tracepath [-m hops] host");
        return;
    }
    route_result_t route;
    if (!route_lookup(dst, &route)) {
        ui_error("tracepath", "Network is unreachable");
        return;
    }
    int pmtu = route.dev->mtu;
    ui_printf(" 1?: [LOCALHOST]                      pmtu %d\n", pmtu);

    icmp_set_observer(on_icmp);
    int hops = 0, reached = 0;
    for (int ttl = 1; ttl <= max_hops && !reached && !signal_interrupted(); ttl++) {
        answered = 0;
        probe_seq = (uint16_t)ttl;
        uint64_t start = tsc_now();
        uint32_t lock = netdev_lock();
        icmp_send_echo(dst, PROBE_ID, probe_seq, (uint8_t)ttl, "tracepath", 9);
        netdev_unlock(lock);
        for (int w = 0; !answered && w < WAIT_MS && !signal_interrupted(); w += 10)
            sys_sleep_interruptible(10);
        ui_printf(" %2d:  ", ttl);
        if (!answered) {
            ui_printf("no reply\n");
            continue;
        }
        char from[INET_ADDR_TEXT];
        inet_format(answer_from, from);
        uint32_t us = tsc_cycles_to_us(answer_tsc - start);
        ui_printf("%-32s %u.%03ums", from, us / 1000, us % 1000);
        hops = ttl;
        if (answer_type == ICMP_ECHO_REPLY) {
            reached = 1;
            ui_color(UI_GOOD);
            ui_printf(" reached");
            ui_reset();
        }
        ui_printf("\n");
    }
    icmp_set_observer(0);
    ui_printf("     Resume: pmtu %d hops %d back %d\n", pmtu, hops, hops);
}
