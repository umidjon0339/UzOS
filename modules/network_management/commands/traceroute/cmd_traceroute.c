/*
 * cmd_traceroute.c - `traceroute`: which routers lie between us and a host?
 *
 *   traceroute 10.0.2.2
 *   traceroute to 10.0.2.2 (10.0.2.2), 30 hops max, 84 byte packets
 *    1  10.0.2.2  0.312 ms  0.201 ms  0.198 ms
 *
 * The trick is the TTL. Every router takes one off and, at 0, throws
 * the packet away and sends back "time exceeded" FROM ITS OWN ADDRESS.
 * So we send echo requests with TTL 1, 2, 3, ...: the router at hop n
 * answers the TTL-n probe. When the host itself answers (echo reply),
 * we have arrived. '*' means no answer within a second.
 *
 * (Linux's traceroute sends UDP by default; this is `traceroute -I`.)
 * Under QEMU's user network the gateway answers for the whole internet,
 * so every path looks one hop long.
 *
 * Uses: icmp_send_echo / icmp_set_observer (core/icmp), dns_resolve (core/dns), inet_* (core/inet),
 *       netdev_lock (core/netdev), tsc_now / tsc_cycles_to_us
 *       (cpu_management), sys_sleep_interruptible / signal_interrupted
 *       (process_management), arg_next, atoi, strcmp, ui_*.
 */

#include "modules/network_management/commands/traceroute/cmd_traceroute.h"  // this command
#include "modules/network_management/core/icmp/icmp.h"               // icmp_send_echo, icmp_set_observer
#include "modules/network_management/core/netdev/netdev.h"           // netdev_lock
#include "modules/network_management/core/inet/inet.h"               // inet_parse, inet_format, NET_E*
#include "modules/network_management/core/dns/dns.h"                 // dns_resolve
#include "modules/cpu_management/core/tsc/tsc.h"                     // tsc_now, tsc_cycles_to_us
#include "modules/process_management/syscalls/sleep/sys_sleep.h"     // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"           // signal_interrupted
#include "lib/args/args.h"                                           // arg_next
#include "lib/stdlib/stdlib.h"                                       // atoi
#include "lib/string/string.h"                                       // strcmp
#include "modules/ui/core/style/ui.h"                                // ui_*

#define USAGE      "traceroute [-m max_hops] [-q probes] host"
#define DATA_LEN   56
#define WAIT_MS    1000
#define STEP_MS    10

// The answer to the probe in flight, filled in the interrupt.
static volatile int answered;
static volatile uint8_t answer_type;
static volatile uint32_t answer_from;
static volatile uint64_t answer_tsc;
static uint16_t probe_id, probe_seq;

static void on_icmp(const icmp_event_t *e) {
    if (answered || e->id != probe_id || e->seq != probe_seq)
        return;
    if (e->type != ICMP_ECHO_REPLY && e->type != ICMP_TIME_EXCEEDED && e->type != ICMP_DEST_UNREACH)
        return;
    answer_type = e->type;
    answer_from = e->from;
    answer_tsc = tsc_now();
    answered = 1;
}

void cmd_traceroute(const char *args) {
    char word[24], host[24] = "";
    int max_hops = 30, probes = 3;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-m") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            max_hops = atoi(word);
        } else if (strcmp(word, "-q") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            probes = atoi(word);
        } else {
            strcpy(host, word);
        }
    }
    uint32_t dst = 0;
    if (!host[0] || max_hops < 1 || max_hops > 64 || probes < 1 || probes > 5) {
        ui_usage(USAGE);
        return;
    }
    int rcode = dns_resolve(host, &dst);       // a name, or already an address
    if (rcode != DNS_OK) {
        ui_start(UI_ERROR, "traceroute");
        ui_printf("%s: %s", host, dns_error_text(rcode));
        ui_end();
        return;
    }
    char addr[INET_ADDR_TEXT];
    inet_format(dst, addr);

    static uint8_t data[DATA_LEN];
    static uint16_t runs;
    probe_id = (uint16_t)(0x7400 + ++runs);
    ui_printf("traceroute to %s (%s), %d hops max, %d byte packets\n", host, addr, max_hops,
              DATA_LEN + 28);
    icmp_set_observer(on_icmp);

    int arrived = 0;
    for (int ttl = 1; ttl <= max_hops && !arrived && !signal_interrupted(); ttl++) {
        ui_printf("%2d ", ttl);
        uint32_t last_from = 0;
        for (int i = 0; i < probes && !signal_interrupted(); i++) {
            answered = 0;
            probe_seq = (uint16_t)(ttl * 8 + i);
            uint64_t start = tsc_now();
            uint32_t lock = netdev_lock();
            int err = icmp_send_echo(dst, probe_id, probe_seq, (uint8_t)ttl, data, DATA_LEN);
            netdev_unlock(lock);
            if (err == -NET_ENETUNREACH) {
                ui_printf("\n");
                ui_error("traceroute", "Network is unreachable (no route)");
                icmp_set_observer(0);
                return;
            }
            for (int waited = 0; !answered && waited < WAIT_MS && !signal_interrupted(); waited += STEP_MS)
                sys_sleep_interruptible(STEP_MS);
            if (!answered) {
                ui_printf(" *");
                continue;
            }
            if (answer_from != last_from) {     // print who answered, once per hop
                char from[INET_ADDR_TEXT];
                inet_format(answer_from, from);
                ui_color(UI_LINK);
                ui_printf(" %s", from);
                ui_reset();
                last_from = answer_from;
            }
            uint32_t us = tsc_cycles_to_us(answer_tsc - start);
            ui_printf("  %u.%03u ms", us / 1000, us % 1000);
            if (answer_type != ICMP_TIME_EXCEEDED)
                arrived = 1;                    // the host itself (or "unreachable")
        }
        ui_printf("\n");
    }
    icmp_set_observer(0);
}
