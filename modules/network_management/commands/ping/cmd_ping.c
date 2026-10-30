/*
 * cmd_ping.c - `ping`: is that host there, and how far away (in time)?
 *
 *   ping 10.0.2.2
 *   PING 10.0.2.2 (10.0.2.2) 56(84) bytes of data.
 *   64 bytes from 10.0.2.2: icmp_seq=1 ttl=255 time=0.412 ms
 *   ...
 *   --- 10.0.2.2 ping statistics ---
 *   4 packets transmitted, 4 received, 0% packet loss
 *   rtt min/avg/max = 0.117/0.300/0.512 ms
 *
 *   -c count     how many (default 4; Ctrl+C stops early)
 *   -s size      data bytes in each request (default 56: 64 with the ICMP header)
 *   -t ttl       how many routers it may pass
 *   -i seconds   time between requests (default 1)
 *
 * Each request carries our id and a sequence number; the reply comes
 * back with the same two, so we know which request it answers. Replies
 * arrive in the card's interrupt: the observer writes them into a small
 * queue and this loop prints them.
 *
 * Uses: icmp_send_echo / icmp_set_observer (core/icmp), dns_resolve (core/dns), inet_* (core/inet),
 *       netdev_lock (core/netdev), tsc_now / tsc_cycles_to_us
 *       (cpu_management), sys_sleep_interruptible / signal_interrupted
 *       (process_management), arg_next, atoi, strcmp, ui_*.
 */

#include "modules/network_management/commands/ping/cmd_ping.h"       // this command
#include "modules/network_management/core/icmp/icmp.h"               // icmp_send_echo, icmp_set_observer
#include "modules/network_management/core/ipv4/ipv4.h"               // IPV4_HLEN
#include "modules/network_management/core/netdev/netdev.h"           // netdev_lock
#include "modules/network_management/core/inet/inet.h"               // inet_parse, inet_format, NET_E*
#include "modules/network_management/core/dns/dns.h"                 // dns_resolve
#include "modules/cpu_management/core/tsc/tsc.h"                     // tsc_now, tsc_cycles_to_us
#include "modules/process_management/syscalls/sleep/sys_sleep.h"     // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"           // signal_interrupted
#include "lib/args/args.h"                                           // arg_next
#include "lib/stdlib/stdlib.h"                                       // atoi
#include "lib/string/string.h"                                       // strcmp, memset
#include "modules/ui/core/style/ui.h"                                // ui_*

#define USAGE        "ping [-c count] [-s size] [-t ttl] [-i seconds] host"
#define MAX_DATA     1472           // 1500 - 20 (IP) - 8 (ICMP): one frame
#define STEP_MS      10
#define REPLY_WAIT_MS 1000          // after the last request
#define SLOTS        64             // send times remembered, by seq

// Events from the interrupt, waiting to be printed.
typedef struct {
    icmp_event_t event;
    uint64_t arrived;               // TSC when it came in
} heard_t;

static heard_t heard[16];
static volatile int heard_in, heard_out;
static uint16_t my_id;
static uint64_t sent_at[SLOTS];

static void on_icmp(const icmp_event_t *e) {
    if (e->id != my_id || (heard_in + 1) % 16 == heard_out)
        return;                     // someone else's, or the queue is full
    heard[heard_in].event = *e;
    heard[heard_in].arrived = tsc_now();
    heard_in = (heard_in + 1) % 16;
}

typedef struct {
    int received, errors;
    uint32_t min_us, max_us, sum_us;
} totals_t;

static void print_us(uint32_t us) {
    ui_printf("%u.%03u", us / 1000, us % 1000);
}

// Print what came in since last time.
static void report(totals_t *t) {
    char from[INET_ADDR_TEXT];
    while (heard_out != heard_in) {
        heard_t *h = &heard[heard_out];
        heard_out = (heard_out + 1) % 16;
        inet_format(h->event.from, from);
        if (h->event.type == ICMP_ECHO_REPLY) {
            uint32_t us = tsc_cycles_to_us(h->arrived - sent_at[h->event.seq % SLOTS]);
            t->received++;
            t->sum_us += us;
            if (t->received == 1 || us < t->min_us) t->min_us = us;
            if (us > t->max_us) t->max_us = us;
            ui_printf("%u bytes from %s: icmp_seq=%u ttl=%u time=", h->event.data_len + ICMP_HLEN,
                      from, h->event.seq, h->event.ttl);
            print_us(us);
            ui_printf(" ms\n");
        } else {
            t->errors++;
            ui_color(UI_WARNING);
            ui_printf("From %s icmp_seq=%u %s\n", from, h->event.seq,
                      h->event.type == ICMP_TIME_EXCEEDED ? "Time to live exceeded" :
                      h->event.code == ICMP_HOST_UNREACH ? "Destination Host Unreachable" :
                      h->event.code == ICMP_PORT_UNREACH ? "Destination Port Unreachable" :
                      "Destination Unreachable");
            ui_reset();
        }
    }
}

// "0.2" -> 200 ms, "2" -> 2000 ms
static int parse_seconds_ms(const char *text) {
    int ms = 0, scale = 1000, fraction = 0;
    for (; *text; text++) {
        if (*text == '.') {
            fraction = 1;
        } else if (*text >= '0' && *text <= '9') {
            if (!fraction) {
                ms = ms * 10 + (*text - '0') * 1000;
            } else if (scale > 1) {
                scale /= 10;
                ms += (*text - '0') * scale;
            }
        } else {
            return -1;
        }
    }
    return ms;
}

void cmd_ping(const char *args) {
    char word[24], host[24] = "";
    int count = 4, size = 56, ttl = 0, interval_ms = 1000;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (word[0] == '-' && word[1] && !word[2]) {
            char option = word[1];
            rest = arg_next(rest, word, sizeof(word));
            if (option == 'c')      count = atoi(word);
            else if (option == 's') size = atoi(word);
            else if (option == 't') ttl = atoi(word);
            else if (option == 'i') interval_ms = parse_seconds_ms(word);
            else count = -1;
        } else {
            strcpy(host, word);
        }
    }
    uint32_t dst = 0;
    if (!host[0] || count <= 0 || size < 0 || size > MAX_DATA ||
        ttl < 0 || ttl > 255 || interval_ms < 200) {
        ui_usage(USAGE);
        if (interval_ms >= 0 && interval_ms < 200)
            ui_hint("intervals below 0.2 s are a flood: not allowed");
        return;
    }

    int rcode = dns_resolve(host, &dst);       // a name, or already an address
    if (rcode != DNS_OK) {
        ui_start(UI_ERROR, "ping");
        ui_printf("%s: %s", host, dns_error_text(rcode));
        ui_end();
        return;
    }
    char addr[INET_ADDR_TEXT];
    inet_format(dst, addr);

    static uint8_t data[MAX_DATA];
    for (int i = 0; i < size; i++)
        data[i] = (uint8_t)(0x10 + i);      // a pattern, as Linux sends

    static uint16_t runs;
    my_id = (uint16_t)(0x5500 + ++runs);
    heard_in = heard_out = 0;
    totals_t t;
    memset(&t, 0, sizeof(t));

    ui_printf("PING %s (%s) %d(%d) bytes of data.\n", host, addr, size, size + ICMP_HLEN + IPV4_HLEN);
    icmp_set_observer(on_icmp);
    int sent = 0;
    for (int seq = 1; seq <= count && !signal_interrupted(); seq++) {
        sent_at[seq % SLOTS] = tsc_now();
        uint32_t lock = netdev_lock();
        int err = icmp_send_echo(dst, my_id, (uint16_t)seq, (uint8_t)ttl, data, (uint16_t)size);
        netdev_unlock(lock);
        if (err == -NET_ENETUNREACH) {
            ui_error("ping", "connect: Network is unreachable (no route: ip route)");
            break;
        }
        sent++;
        // Wait the interval (the last time: long enough for its reply).
        int wait = seq == count ? REPLY_WAIT_MS : interval_ms;
        for (int waited = 0; waited < wait && !signal_interrupted(); waited += STEP_MS) {
            report(&t);
            if (seq == count && t.received + t.errors >= sent)
                break;                      // the last answer is in
            sys_sleep_interruptible(STEP_MS);
        }
        report(&t);
    }
    icmp_set_observer(0);
    if (!sent)
        return;

    ui_printf("--- %s ping statistics ---\n", host);
    int loss = (sent - t.received) * 100 / sent;
    ui_printf("%d packets transmitted, %d received, ", sent, t.received);
    if (t.errors)
        ui_printf("+%d errors, ", t.errors);
    ui_color(loss ? UI_BAD : UI_GOOD);
    ui_printf("%d%% packet loss\n", loss);
    ui_reset();
    if (t.received) {
        ui_printf("rtt min/avg/max = ");
        print_us(t.min_us);
        ui_printf("/");
        print_us(t.sum_us / t.received);
        ui_printf("/");
        print_us(t.max_us);
        ui_printf(" ms\n");
    }
}
