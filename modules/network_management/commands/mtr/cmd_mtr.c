/*
 * cmd_mtr.c - `mtr`: probe every hop again and again, then report.
 *
 *   mtr -c 5 example.com
 *   HOST: uzos                 Loss%   Snt   Last    Avg   Best   Wrst
 *     1.|-- 10.0.2.2            0.0%     5    0.4    0.5    0.3    1.0
 *
 * Each cycle sends one echo request per TTL (1, 2, ... up to the hop
 * that answers as the host itself). Over many cycles a hop that drops
 * packets, or a slow link, stands out. This is mtr's "--report" mode.
 *
 * Uses: icmp_send_echo / icmp_set_observer (core/icmp), dns_resolve
 *       (core/dns), hostname_get (core/hostname), inet_format (core/inet),
 *       tsc_now (cpu_management), sys_sleep_interruptible /
 *       signal_interrupted (process_management), arg_next, atoi, ui_*.
 */

#include "modules/network_management/commands/mtr/cmd_mtr.h"        // this command
#include "modules/network_management/core/icmp/icmp.h"              // icmp_send_echo, icmp_set_observer
#include "modules/network_management/core/netdev/netdev.h"          // netdev_lock
#include "modules/network_management/core/dns/dns.h"                // dns_resolve
#include "modules/network_management/core/hostname/hostname.h"      // hostname_get
#include "modules/network_management/core/inet/inet.h"              // inet_format
#include "modules/cpu_management/core/tsc/tsc.h"                    // tsc_now, tsc_cycles_to_us
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"          // signal_interrupted
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp, strcpy, memset
#include "modules/ui/core/style/ui.h"                               // ui_*

#define MAX_HOPS  16
#define WAIT_MS   1000
#define PROBE_ID  0x6D74            // "mt"

typedef struct {
    uint32_t addr;
    int sent, received;
    uint32_t last_us, sum_us, best_us, worst_us;
} hop_t;

static hop_t hops[MAX_HOPS];
static volatile int got[MAX_HOPS];          // this cycle's answer per hop
static volatile uint32_t from[MAX_HOPS];
static volatile uint64_t when[MAX_HOPS];
static volatile int final_ttl;              // the TTL at which the host itself answered

static void on_icmp(const icmp_event_t *e) {
    if (e->id != PROBE_ID || e->seq < 1 || e->seq > MAX_HOPS)
        return;
    int i = e->seq - 1;
    if (got[i])
        return;
    from[i] = e->from;
    when[i] = tsc_now();
    got[i] = 1;
    if (e->type == ICMP_ECHO_REPLY && (!final_ttl || e->seq < final_ttl))
        final_ttl = e->seq;
}

static void print_ms(uint32_t us) {
    ui_printf(" %4u.%u", us / 1000, (us % 1000) / 100);
}

void cmd_mtr(const char *args) {
    char word[DNS_NAME_MAX], host[DNS_NAME_MAX] = "";
    int cycles = 5;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-c") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            cycles = atoi(word);
        } else if (strcmp(word, "-r") != 0 && strcmp(word, "--report") != 0) {
            strcpy(host, word);
        }
    }
    uint32_t dst;
    if (!host[0] || cycles < 1 || cycles > 100 || dns_resolve(host, &dst) != DNS_OK) {
        ui_usage("mtr [-c cycles] host");
        return;
    }
    memset(hops, 0, sizeof(hops));
    final_ttl = 0;
    icmp_set_observer(on_icmp);
    ui_printf("mtr: %d cycles to %s ...\n", cycles, host);
    for (int cycle = 0; cycle < cycles && !signal_interrupted(); cycle++) {
        int last = final_ttl ? final_ttl : MAX_HOPS;
        uint64_t start = tsc_now();
        for (int ttl = 1; ttl <= last; ttl++) {
            got[ttl - 1] = 0;
            hops[ttl - 1].sent++;
            uint32_t lock = netdev_lock();
            icmp_send_echo(dst, PROBE_ID, (uint16_t)ttl, (uint8_t)ttl, "mtr", 3);
            netdev_unlock(lock);
        }
        for (int w = 0; w < WAIT_MS && !signal_interrupted(); w += 10)
            sys_sleep_interruptible(10);
        for (int ttl = 1; ttl <= last; ttl++) {
            hop_t *h = &hops[ttl - 1];
            if (!got[ttl - 1])
                continue;
            uint32_t us = tsc_cycles_to_us(when[ttl - 1] - start);
            h->addr = from[ttl - 1];
            h->received++;
            h->last_us = us;
            h->sum_us += us;
            if (h->received == 1 || us < h->best_us) h->best_us = us;
            if (us > h->worst_us) h->worst_us = us;
        }
    }
    icmp_set_observer(0);

    ui_color(UI_TITLE);
    ui_printf("HOST: %-20s Loss%%   Snt   Last    Avg   Best   Wrst\n", hostname_get());
    ui_reset();
    int last = final_ttl ? final_ttl : MAX_HOPS;
    for (int ttl = 1; ttl <= last; ttl++) {
        hop_t *h = &hops[ttl - 1];
        char addr[INET_ADDR_TEXT] = "???";
        if (h->received)
            inet_format(h->addr, addr);
        int loss10 = h->sent ? (h->sent - h->received) * 1000 / h->sent : 0;   // tenths of a percent
        ui_printf("%3d.|-- %-17s ", ttl, addr);
        ui_color(loss10 ? UI_WARNING : UI_GOOD);
        ui_printf("%3d.%d%%", loss10 / 10, loss10 % 10);
        ui_reset();
        ui_printf(" %5d", h->sent);
        if (h->received) {
            print_ms(h->last_us);
            print_ms(h->sum_us / h->received);
            print_ms(h->best_us);
            print_ms(h->worst_us);
        }
        ui_printf("\n");
    }
}
