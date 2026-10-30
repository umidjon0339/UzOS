/*
 * cmd_arping.c - `arping [-c count] [-I dev] IP`: is IP on my link?
 *
 *   arping 10.0.2.2
 *   ARPING 10.0.2.2 from 10.0.2.15 eth0
 *   Unicast reply from 10.0.2.2 [52:55:0a:00:02:02]  0.214ms
 *   ...
 *   Sent 3 probes, received 3 responses
 *
 * Unlike ping it works one level lower: no IP route is needed, only
 * the link, and only machines on the same link can answer. One request
 * per second; Ctrl+C stops early. The reply time is measured with the
 * CPU's cycle counter, for microseconds.
 *
 * Uses: arp_request / arp_set_observer (core/arp), netdev_* (core/netdev),
 *       inet_* / mac_format (core/inet), tsc_now / tsc_cycles_to_us
 *       (cpu_management), sys_sleep_interruptible / signal_interrupted
 *       (process_management), arg_next, atoi, strcmp, ui_*.
 */

#include "modules/network_management/commands/arping/cmd_arping.h"  // this command
#include "modules/network_management/core/arp/arp.h"                 // arp_request, arp_set_observer
#include "modules/network_management/core/netdev/netdev.h"           // netdev_by_name, netdev_lock
#include "modules/network_management/core/inet/inet.h"               // inet_parse, inet_format, mac_format
#include "modules/cpu_management/core/tsc/tsc.h"                     // tsc_now, tsc_cycles_to_us
#include "modules/process_management/syscalls/sleep/sys_sleep.h"     // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"           // signal_interrupted
#include "lib/args/args.h"                                           // arg_next
#include "lib/stdlib/stdlib.h"                                       // atoi
#include "lib/string/string.h"                                       // strcmp, memcpy
#include "modules/ui/core/style/ui.h"                                // ui_*

#define USAGE        "arping [-c count] [-I dev] IP"
#define DEFAULT_COUNT 3
#define WAIT_MS      1000       // time for each answer
#define STEP_MS      10

// Filled by the observer, inside the card's interrupt.
static volatile uint32_t wanted_ip;
static volatile int answered;
static volatile uint64_t answer_tsc;
static uint8_t answer_mac[6];

static void on_reply(uint32_t ip, const uint8_t mac[6]) {
    if (ip != wanted_ip || answered)
        return;
    answer_tsc = tsc_now();
    memcpy(answer_mac, mac, 6);
    answered = 1;
}

void cmd_arping(const char *args) {
    char word[20], name[NETDEV_NAME_LEN] = "eth0";
    int count = DEFAULT_COUNT;
    uint32_t ip = 0;
    int have_ip = 0;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-c") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            count = atoi(word);
        } else if (strcmp(word, "-I") == 0) {
            rest = arg_next(rest, name, sizeof(name));
        } else {
            have_ip = inet_parse(word, &ip);
        }
    }
    netdev_t *dev = netdev_by_name(name);
    if (!have_ip || count <= 0 || !dev) {
        ui_usage(USAGE);
        return;
    }
    if (!netdev_running(dev)) {
        ui_error("arping", "the interface is down");
        return;
    }

    char ip_text[INET_ADDR_TEXT], from[INET_ADDR_TEXT], mac[MAC_TEXT];
    inet_format(ip, ip_text);
    inet_format(dev->ip, from);
    ui_printf("ARPING %s from %s %s\n", ip_text, from, dev->name);

    wanted_ip = ip;
    arp_set_observer(on_reply);
    int sent = 0, received = 0;
    for (int i = 0; i < count && !signal_interrupted(); i++) {
        answered = 0;
        uint64_t start = tsc_now();
        uint32_t lock = netdev_lock();
        arp_request(dev, ip);
        netdev_unlock(lock);
        sent++;

        int waited = 0;
        while (!answered && waited < WAIT_MS && !signal_interrupted()) {
            sys_sleep_interruptible(STEP_MS);
            waited += STEP_MS;
        }
        if (answered) {
            received++;
            uint32_t us = tsc_cycles_to_us(answer_tsc - start);
            mac_format(answer_mac, mac);
            ui_printf("Unicast reply from %s [%s]  %u.%03ums\n", ip_text, mac, us / 1000, us % 1000);
            if (i + 1 < count)
                sys_sleep_interruptible(WAIT_MS - waited);   // one probe per second
        } else if (!signal_interrupted()) {
            ui_color(UI_WARNING);
            ui_printf("Timeout\n");
            ui_reset();
        }
    }
    arp_set_observer(0);
    ui_printf("Sent %d probes, received %d responses\n", sent, received);
}
