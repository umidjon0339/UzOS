/*
 * cmd_tcpdump.c - `tcpdump`: see the packets themselves.
 *
 *   tcpdump                    eth0, until Ctrl+C
 *   tcpdump -i any -c 10       every interface, 10 packets
 *   tcpdump icmp               only ICMP (also: arp, ip, tcp, udp)
 *   tcpdump port 80            only packets from or to port 80
 *   tcpdump host 10.0.2.2      only packets from or to that address
 *   tcpdump -A port 23         and the data as text: a telnet password
 *                              is plainly readable (security/plaintext_sniffing)
 *   -e  the MAC addresses too     -X  the data in hex and text
 *   -p  do not turn on promiscuous mode
 *
 *   12.345 IP 10.0.2.15.32768 > 93.184.215.14.80: Flags [S], seq 4021, win 8192, length 0
 *
 * A tap in netdev (netdev_set_tap) sees every frame, in or out, before
 * anything filters it. It runs in the card's interrupt, so it only
 * copies the first SNAP bytes into a ring; this loop decodes and
 * prints them. Promiscuous mode makes the card keep frames for OTHER
 * machines too: that is why tcpdump needs CAP_NET_RAW (root).
 *
 * Uses: netdev_set_tap / netdev_* (core/netdev), ethernet / ipv4 / arp /
 *       tcp / udp / icmp headers (core), net_now_ms (core/net_tick),
 *       caps_require (security_management), inet_* (core/inet),
 *       sys_sleep_interruptible / signal_interrupted, arg_next, ui_*.
 */

#include "modules/network_management/commands/tcpdump/cmd_tcpdump.h"  // this command
#include "modules/network_management/core/netdev/netdev.h"          // netdev_set_tap, netdev_*
#include "modules/network_management/core/ethernet/ethernet.h"      // eth_header_t, ETH_P_*
#include "modules/network_management/core/arp/arp.h"                // arp_packet_t
#include "modules/network_management/core/ipv4/ipv4.h"              // ipv4_header_t, IPPROTO_*
#include "modules/network_management/core/icmp/icmp.h"              // icmp_header_t, ICMP_*
#include "modules/network_management/core/tcp/tcp.h"                // tcp_header_t, TCP_*
#include "modules/network_management/core/udp/udp.h"                // udp_header_t
#include "modules/network_management/core/net_tick/net_tick.h"      // net_now_ms
#include "modules/network_management/core/inet/inet.h"              // inet_parse, inet_format, mac_format
#include "modules/security_management/core/caps/caps.h"             // caps_require, CAP_NET_RAW
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"          // signal_interrupted
#include "modules/process_management/core/process/process.h"        // process_caller_uid
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp, memcpy
#include "modules/ui/core/style/ui.h"                               // ui_*

#define RING   64               // frames waiting to be printed
#define SNAP   256              // bytes kept of each frame (tcpdump's "snaplen")
#define STEP_MS 10

typedef struct {
    netdev_t *dev;
    uint16_t len;               // the whole frame
    uint16_t kept;              // bytes in 'bytes'
    int outgoing;
    uint32_t ms;
    uint8_t bytes[SNAP];
} frame_t;

static frame_t ring[RING];
static volatile int ring_in, ring_out;
static volatile uint32_t lost;              // the ring was full
static netdev_t *only_dev;                  // NULL = any

// In the interrupt: copy, nothing else.
static void tap(netdev_t *dev, const uint8_t *data, uint16_t len, int outgoing) {
    if (only_dev && dev != only_dev)
        return;
    int next = (ring_in + 1) % RING;
    if (next == ring_out) {
        lost++;
        return;
    }
    frame_t *f = &ring[ring_in];
    f->dev = dev;
    f->len = len;
    f->kept = len < SNAP ? len : SNAP;
    f->outgoing = outgoing;
    f->ms = net_now_ms();
    memcpy(f->bytes, data, f->kept);
    ring_in = next;
}

/* ---------- The filter ---------- */

typedef struct {
    int proto;                  // 0 any, -1 arp, -2 ip, else an IPPROTO_*
    uint16_t port;
    uint32_t host;
    int ascii, hex, macs;
} filter_t;

static int wanted(const filter_t *f, const frame_t *fr) {
    if (fr->kept < ETH_HLEN)
        return 0;
    const eth_header_t *eth = (const eth_header_t *)fr->bytes;
    uint16_t type = ntohs(eth->type);
    if (f->proto == -1)
        return type == ETH_P_ARP && !f->port && !f->host;
    if (type != ETH_P_IP)
        return f->proto == 0 && !f->port && !f->host;
    const ipv4_header_t *ip = (const ipv4_header_t *)(fr->bytes + ETH_HLEN);
    if (f->proto > 0 && ip->protocol != f->proto)
        return 0;
    if (f->host && ntohl(ip->src) != f->host && ntohl(ip->dst) != f->host)
        return 0;
    if (f->port) {
        uint16_t hlen = (uint16_t)((ip->version_ihl & 0x0F) * 4);
        if ((ip->protocol != IPPROTO_TCP && ip->protocol != IPPROTO_UDP) || fr->kept < ETH_HLEN + hlen + 4)
            return 0;
        const uint8_t *t = fr->bytes + ETH_HLEN + hlen;
        uint16_t sport = (uint16_t)((t[0] << 8) | t[1]), dport = (uint16_t)((t[2] << 8) | t[3]);
        if (sport != f->port && dport != f->port)
            return 0;
    }
    return 1;
}

/* ---------- Printing one frame ---------- */

static void tcp_flags(uint8_t flags) {
    ui_printf("Flags [");
    if (flags & TCP_SYN) ui_printf("S");
    if (flags & TCP_FIN) ui_printf("F");
    if (flags & TCP_RST) ui_printf("R");
    if (flags & TCP_PSH) ui_printf("P");
    if (flags & TCP_ACK) ui_printf(".");
    ui_printf("]");
}

static void payload(const filter_t *f, const uint8_t *data, int len) {
    if (len <= 0 || (!f->ascii && !f->hex))
        return;
    for (int row = 0; row < len; row += 16) {
        ui_color(UI_DIM);
        if (f->hex) {
            ui_printf("  0x%04x: ", row);
            for (int i = row; i < row + 16; i++) {
                if (i < len)
                    ui_printf("%02x", data[i]);
                else
                    ui_printf("  ");                // keep the text column in line
                if (i % 2)
                    ui_printf(" ");
            }
        }
        for (int i = row; i < row + 16 && i < len; i++)
            ui_printf("%c", data[i] >= 32 && data[i] < 127 ? data[i] : '.');
        ui_printf("\n");
        ui_reset();
    }
}

static void print_ip(const filter_t *f, const frame_t *fr) {
    const ipv4_header_t *ip = (const ipv4_header_t *)(fr->bytes + ETH_HLEN);
    uint16_t hlen = (uint16_t)((ip->version_ihl & 0x0F) * 4);
    uint16_t total = ntohs(ip->total_length);
    char src[INET_ADDR_TEXT], dst[INET_ADDR_TEXT];
    inet_format(ntohl(ip->src), src);
    inet_format(ntohl(ip->dst), dst);
    const uint8_t *t = fr->bytes + ETH_HLEN + hlen;
    int avail = fr->kept - ETH_HLEN - hlen;              // transport bytes we kept
    int frag = ntohs(ip->frag) & (IPV4_FLAG_MF | IPV4_OFFSET_MASK);

    if (ip->protocol == IPPROTO_TCP && !frag && avail >= TCP_HLEN) {
        const tcp_header_t *h = (const tcp_header_t *)t;
        uint16_t thlen = (uint16_t)((h->offset >> 4) * 4);
        int data_len = total - hlen - thlen;
        ui_printf("IP %s.%u > %s.%u: ", src, ntohs(h->src_port), dst, ntohs(h->dst_port));
        tcp_flags(h->flags);
        ui_printf(", seq %u", ntohl(h->seq));
        if (h->flags & TCP_ACK)
            ui_printf(", ack %u", ntohl(h->ack));
        ui_printf(", win %u, length %d\n", ntohs(h->window), data_len);
        payload(f, t + thlen, avail - thlen < data_len ? avail - thlen : data_len);
    } else if (ip->protocol == IPPROTO_UDP && !frag && avail >= UDP_HLEN) {
        const udp_header_t *h = (const udp_header_t *)t;
        ui_printf("IP %s.%u > %s.%u: UDP, length %u\n", src, ntohs(h->src_port), dst,
                  ntohs(h->dst_port), ntohs(h->length) - UDP_HLEN);
        payload(f, t + UDP_HLEN, avail - UDP_HLEN);
    } else if (ip->protocol == IPPROTO_ICMP && !frag && avail >= ICMP_HLEN) {
        const icmp_header_t *h = (const icmp_header_t *)t;
        ui_printf("IP %s > %s: ICMP ", src, dst);
        if (h->type == ICMP_ECHO_REQUEST || h->type == ICMP_ECHO_REPLY)
            ui_printf("echo %s, id %u, seq %u", h->type == ICMP_ECHO_REQUEST ? "request" : "reply",
                      ntohs(h->id), ntohs(h->seq));
        else if (h->type == ICMP_DEST_UNREACH)
            ui_printf("%s unreachable", h->code == ICMP_PORT_UNREACH ? "port" : h->code == ICMP_HOST_UNREACH ? "host" : "destination");
        else if (h->type == ICMP_TIME_EXCEEDED)
            ui_printf("time exceeded in-transit");
        else
            ui_printf("type %u code %u", h->type, h->code);
        ui_printf(", length %u\n", total - hlen);
    } else {
        ui_printf("IP %s > %s: proto %u%s, length %u\n", src, dst, ip->protocol,
                  frag ? " (fragment)" : "", total - hlen);
    }
}

static void print_frame(const filter_t *f, const frame_t *fr) {
    const eth_header_t *eth = (const eth_header_t *)fr->bytes;
    ui_color(UI_DIM);
    ui_printf("%u.%03u ", fr->ms / 1000, fr->ms % 1000);
    if (!only_dev)
        ui_printf("%s %s ", fr->dev->name, fr->outgoing ? "Out" : "In ");
    ui_reset();
    if (f->macs) {
        char a[MAC_TEXT], b[MAC_TEXT];
        mac_format(eth->src, a);
        mac_format(eth->dest, b);
        ui_printf("%s > %s, ", a, b);
    }
    uint16_t type = ntohs(eth->type);
    if (type == ETH_P_ARP && fr->kept >= ETH_HLEN + (int)sizeof(arp_packet_t)) {
        const arp_packet_t *a = (const arp_packet_t *)(fr->bytes + ETH_HLEN);
        char sender[INET_ADDR_TEXT], target[INET_ADDR_TEXT], mac[MAC_TEXT];
        inet_format(ntohl(a->sender_ip), sender);
        inet_format(ntohl(a->target_ip), target);
        mac_format(a->sender_mac, mac);
        if (ntohs(a->op) == ARP_OP_REQUEST)
            ui_printf("ARP, Request who-has %s tell %s, length 28\n", target, sender);
        else
            ui_printf("ARP, Reply %s is-at %s, length 28\n", sender, mac);
    } else if (type == ETH_P_IP && fr->kept >= ETH_HLEN + IPV4_HLEN) {
        print_ip(f, fr);
    } else {
        ui_printf("ethertype 0x%04x, length %u\n", type, fr->len);
    }
}

/* ---------- The command ---------- */

void cmd_tcpdump(const char *args) {
    filter_t f = { 0 };
    char word[24], dev_name[NETDEV_NAME_LEN] = "eth0";
    int count = 0, promisc = 1;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-i") == 0)         rest = arg_next(rest, dev_name, sizeof(dev_name));
        else if (strcmp(word, "-c") == 0)  { rest = arg_next(rest, word, sizeof(word)); count = atoi(word); }
        else if (strcmp(word, "-A") == 0)    f.ascii = 1;
        else if (strcmp(word, "-X") == 0)    f.hex = f.ascii = 1;
        else if (strcmp(word, "-e") == 0)    f.macs = 1;
        else if (strcmp(word, "-p") == 0)    promisc = 0;
        else if (strcmp(word, "-n") == 0 || strcmp(word, "and") == 0) continue;
        else if (strcmp(word, "arp") == 0)   f.proto = -1;
        else if (strcmp(word, "ip") == 0)    f.proto = -2;
        else if (strcmp(word, "icmp") == 0)  f.proto = IPPROTO_ICMP;
        else if (strcmp(word, "tcp") == 0)   f.proto = IPPROTO_TCP;
        else if (strcmp(word, "udp") == 0)   f.proto = IPPROTO_UDP;
        else if (strcmp(word, "port") == 0){ rest = arg_next(rest, word, sizeof(word)); f.port = (uint16_t)atoi(word); }
        else if (strcmp(word, "host") == 0){ rest = arg_next(rest, word, sizeof(word)); inet_parse(word, &f.host); }
        else {
            ui_usage("tcpdump [-i dev|any] [-c N] [-A] [-X] [-e] [-p] [arp|ip|icmp|tcp|udp] [port N] [host IP]");
            return;
        }
    }
    if (f.proto == -2)
        f.proto = 0;                            // "ip": any IP protocol (ARP is filtered below)
    netdev_t *dev = strcmp(dev_name, "any") == 0 ? 0 : netdev_by_name(dev_name);
    if (strcmp(dev_name, "any") != 0 && !dev) {
        ui_error("tcpdump", "no such interface");
        return;
    }
    // Watching other people's traffic is a root privilege.
    if (!caps_require(CAP_NET_RAW, "tcpdump"))
        return;

    only_dev = dev;
    ring_in = ring_out = 0;
    lost = 0;
    int was_promisc = dev && (dev->flags & NETDEV_PROMISC);
    uint32_t lock = netdev_lock();
    if (dev && promisc && !was_promisc)
        netdev_set_promisc(dev, 1, process_caller_uid());
    netdev_set_tap(tap);
    netdev_unlock(lock);
    ui_printf("tcpdump: listening on %s%s, Ctrl+C to stop\n", dev ? dev->name : "any",
              dev && promisc ? " (promiscuous)" : "");

    int shown = 0, seen = 0;
    while (!signal_interrupted() && (!count || shown < count)) {
        while (ring_out != ring_in && (!count || shown < count)) {
            frame_t *fr = &ring[ring_out];
            seen++;
            if (wanted(&f, fr)) {
                print_frame(&f, fr);
                shown++;
            }
            ring_out = (ring_out + 1) % RING;
        }
        sys_sleep_interruptible(STEP_MS);
    }

    lock = netdev_lock();
    netdev_set_tap(0);
    if (dev && promisc && !was_promisc)
        netdev_set_promisc(dev, 0, process_caller_uid());
    netdev_unlock(lock);
    ui_printf("\n%d packets captured, %d seen, %u dropped by the ring\n", shown, seen, lost);
}
