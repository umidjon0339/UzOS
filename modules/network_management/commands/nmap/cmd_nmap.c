/*
 * cmd_nmap.c - `nmap`: a port scanner.
 *
 *   nmap localhost              connect scan of common ports (anyone may)
 *   nmap -p 1-100 10.0.2.2      these ports
 *   nmap -sS 10.0.2.2           SYN ("half-open") scan          needs CAP_NET_RAW
 *   nmap -sU -p 53 10.0.2.3     UDP scan                        needs CAP_NET_RAW
 *   nmap -sn 10.0.2.0/24        which hosts answer a ping       needs CAP_NET_RAW
 *
 * How each scan tells the states apart:
 *
 *   -sT  connect()        accepted -> open    refused (RST) -> closed    silence -> filtered
 *   -sS  send a bare SYN  SYN-ACK  -> open    RST           -> closed    silence -> filtered
 *        (never finishes the handshake: our own TCP answers the SYN-ACK
 *        with a RST, since it never asked; old servers did not even log it)
 *   -sU  empty datagram   an answer -> open   ICMP port unreachable -> closed
 *                         silence -> open|filtered (UDP never says "yes")
 *
 * The raw scans build their packets themselves and watch the answers
 * through netdev's tap, as tcpdump does. Scanning is how attacks begin:
 * security/port_scan shows how UzOS notices it.
 *
 * Uses: socket_* (core/socket), ipv4_send / tcp & udp headers (core),
 *       netdev_set_tap (core/netdev), icmp_send_echo / icmp_set_observer
 *       (core/icmp), dns_resolve (core/dns), route_lookup (core/route),
 *       inet_* (core/inet), caps_require (security_management), tsc_now
 *       (cpu_management), sys_sleep_interruptible / signal_interrupted,
 *       arg_next, atoi, format, strcmp, ui_*.
 */

#include "modules/network_management/commands/nmap/cmd_nmap.h"      // this command
#include "modules/network_management/core/socket/socket.h"          // socket_*
#include "modules/network_management/core/netdev/netdev.h"          // netdev_set_tap, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"      // ETH_HLEN, ETH_P_IP
#include "modules/network_management/core/ipv4/ipv4.h"              // ipv4_send, ipv4_header_t
#include "modules/network_management/core/tcp/tcp.h"                // tcp_header_t, TCP_*
#include "modules/network_management/core/udp/udp.h"                // udp_send, udp_header_t
#include "modules/network_management/core/icmp/icmp.h"              // icmp_*
#include "modules/network_management/core/route/route.h"            // route_lookup
#include "modules/network_management/core/dns/dns.h"                // dns_resolve
#include "modules/network_management/core/inet/inet.h"              // inet_*, htons, NET_E*
#include "modules/security_management/core/caps/caps.h"             // caps_require, CAP_NET_RAW
#include "modules/cpu_management/core/tsc/tsc.h"                    // tsc_now
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"          // signal_interrupted
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp, strcpy, memset
#include "lib/format/format.h"                                      // format
#include "modules/ui/core/style/ui.h"                               // ui_*

#define USAGE        "nmap [-sT|-sS|-sU|-sn] [-p N|N-M|N,M,...] host | A.B.C.0/24"
#define MAX_PORTS    128
#define WAIT_MS      1500           // raw scans: how long to wait for the answers
#define CONNECT_MS   1000           // -sT: how long one port may take
#define SCAN_SPORT   47123          // the source port of our raw probes

typedef enum { UNKNOWN, OPEN, CLOSED, FILTERED, OPEN_FILTERED } port_state_t;

static const uint16_t common_ports[] = {
    7, 21, 22, 23, 25, 53, 80, 110, 111, 135, 139, 143, 443, 445, 993, 995,
    3306, 3389, 5900, 8000, 8080,
};

static const struct { uint16_t port; const char *name; } services[] = {
    { 7, "echo" }, { 21, "ftp" }, { 22, "ssh" }, { 23, "telnet" }, { 25, "smtp" }, { 53, "domain" },
    { 67, "dhcps" }, { 68, "dhcpc" }, { 80, "http" }, { 110, "pop3" }, { 111, "rpcbind" },
    { 135, "msrpc" }, { 139, "netbios-ssn" }, { 143, "imap" }, { 443, "https" },
    { 445, "microsoft-ds" }, { 993, "imaps" }, { 995, "pop3s" }, { 3306, "mysql" },
    { 3389, "ms-wbt-server" }, { 5900, "vnc" }, { 8000, "http-alt" }, { 8080, "http-proxy" },
};

static uint16_t ports[MAX_PORTS];
static volatile port_state_t states[MAX_PORTS];
static int port_count;
static volatile uint32_t target;            // the host being scanned (for the tap)

static const char *service(uint16_t port) {
    for (unsigned i = 0; i < sizeof(services) / sizeof(services[0]); i++)
        if (services[i].port == port)
            return services[i].name;
    return "unknown";
}

// "22", "1-100", "22,80,443" -> ports[]
static int parse_ports(const char *text) {
    port_count = 0;
    while (*text && port_count < MAX_PORTS) {
        int first = atoi(text), last = first;
        while (*text >= '0' && *text <= '9')
            text++;
        if (*text == '-') {
            last = atoi(++text);
            while (*text >= '0' && *text <= '9')
                text++;
        }
        if (first < 1 || last > 65535 || last < first)
            return 0;
        for (int p = first; p <= last && port_count < MAX_PORTS; p++)
            ports[port_count++] = (uint16_t)p;
        if (*text == ',')
            text++;
        else if (*text)
            return 0;
    }
    return port_count > 0;
}

/* ---------- -sT: connect() ---------- */

static void connect_scan(uint32_t ip) {
    for (int i = 0; i < port_count && !signal_interrupted(); i++) {
        socket_t *s;
        if (socket_create(AF_INET, SOCK_STREAM, 0, &s) < 0)
            return;
        socket_timeval_t wait = { 0, CONNECT_MS * 1000 };
        socket_setopt(s, SOL_SOCKET, SO_SNDTIMEO, &wait, sizeof(wait));
        sockaddr_t to;
        uint32_t len = sizeof(to);
        socket_inet_addr(&to, &len, ip, ports[i]);
        int r = socket_connect(s, &to, len);
        states[i] = r == 0 ? OPEN : r == -NET_ECONNREFUSED ? CLOSED : FILTERED;
        socket_close(s);
    }
}

/* ---------- Raw scans: send, then watch the tap ---------- */

static int index_of(uint16_t port) {
    for (int i = 0; i < port_count; i++)
        if (ports[i] == port)
            return i;
    return -1;
}

// In the interrupt: answers from the target to our probes.
static void watch(netdev_t *dev, const uint8_t *frame, uint16_t len, int outgoing) {
    (void)dev;
    if (outgoing || len < ETH_HLEN + IPV4_HLEN + 8 || ((frame[12] << 8) | frame[13]) != ETH_P_IP)
        return;
    const ipv4_header_t *ip = (const ipv4_header_t *)(frame + ETH_HLEN);
    uint16_t hlen = (uint16_t)((ip->version_ihl & 0x0F) * 4);
    const uint8_t *t = frame + ETH_HLEN + hlen;
    if (ntohl(ip->src) != target)
        return;
    if (ip->protocol == IPPROTO_TCP && len >= ETH_HLEN + hlen + TCP_HLEN) {
        const tcp_header_t *h = (const tcp_header_t *)t;
        int i = index_of(ntohs(h->src_port));
        if (i >= 0 && ntohs(h->dst_port) == SCAN_SPORT)
            states[i] = (h->flags & TCP_SYN) && (h->flags & TCP_ACK) ? OPEN : (h->flags & TCP_RST) ? CLOSED : states[i];
    } else if (ip->protocol == IPPROTO_UDP) {
        const udp_header_t *h = (const udp_header_t *)t;
        int i = index_of(ntohs(h->src_port));
        if (i >= 0)
            states[i] = OPEN;                   // it answered
    } else if (ip->protocol == IPPROTO_ICMP && t[0] == ICMP_DEST_UNREACH && t[1] == ICMP_PORT_UNREACH &&
               len >= ETH_HLEN + hlen + ICMP_HLEN + IPV4_HLEN + 4) {
        // It quotes our datagram: its destination port is the closed one.
        const uint8_t *quoted = t + ICMP_HLEN;
        uint16_t qhlen = (uint16_t)((quoted[0] & 0x0F) * 4);
        int i = index_of((uint16_t)((quoted[qhlen + 2] << 8) | quoted[qhlen + 3]));
        if (i >= 0)
            states[i] = CLOSED;
    }
}

// A bare SYN from SCAN_SPORT, built by hand.
static void send_syn(uint32_t src, uint32_t dst, uint16_t port, uint32_t seq) {
    packet_t *p = packet_alloc();
    if (!p)
        return;
    tcp_header_t *h = (tcp_header_t *)packet_put(p, TCP_HLEN);
    memset(h, 0, TCP_HLEN);
    h->src_port = htons(SCAN_SPORT);
    h->dst_port = htons(port);
    h->seq = htonl(seq);
    h->offset = (TCP_HLEN / 4) << 4;
    h->flags = TCP_SYN;
    h->window = htons(1024);                    // nmap's usual
    h->checksum = inet_checksum_pseudo(src, dst, IPPROTO_TCP, p->data, p->len);
    ipv4_send(p, src, dst, IPPROTO_TCP, 0);
}

static void raw_scan(uint32_t ip, int udp) {
    route_result_t route;
    if (!route_lookup(ip, &route)) {
        ui_error("nmap", "no route to host");
        return;
    }
    target = ip;
    for (int i = 0; i < port_count; i++)
        states[i] = udp ? OPEN_FILTERED : FILTERED;     // what silence will mean
    uint32_t lock = netdev_lock();
    netdev_set_tap(watch);
    netdev_unlock(lock);
    uint32_t seq = (uint32_t)tsc_now();
    for (int i = 0; i < port_count && !signal_interrupted(); i++) {
        lock = netdev_lock();
        if (udp)
            udp_send(route.src, SCAN_SPORT, ip, ports[i], "", 0, 0);
        else
            send_syn(route.src, ip, ports[i], seq + i);
        netdev_unlock(lock);
        if (i % 8 == 7)
            sys_sleep_interruptible(10);        // let answers (and ARP) in
    }
    for (int waited = 0; waited < WAIT_MS && !signal_interrupted(); waited += 50)
        sys_sleep_interruptible(50);
    lock = netdev_lock();
    netdev_set_tap(0);
    netdev_unlock(lock);
}

/* ---------- -sn: who is up ---------- */

static volatile uint32_t alive[256 / 32];   // bit n = A.B.C.n answered

static void on_reply(const icmp_event_t *e) {
    if (e->type == ICMP_ECHO_REPLY && e->id == 0x6E6D && (e->from & 0xFFFFFF00) == (target & 0xFFFFFF00))
        alive[(e->from & 0xFF) / 32] |= 1u << ((e->from & 0xFF) % 32);
}

static void ping_sweep(uint32_t net, int prefix) {
    int count = prefix >= 32 ? 1 : 1 << (32 - (prefix < 24 ? 24 : prefix));
    target = net;
    memset((void *)alive, 0, sizeof(alive));
    icmp_set_observer(on_reply);
    for (int i = 0; i < count && !signal_interrupted(); i++) {
        uint32_t lock = netdev_lock();
        icmp_send_echo(net + (uint32_t)i, 0x6E6D, (uint16_t)i, 0, "nmap", 4);   // id "nm"
        netdev_unlock(lock);
        if (i % 8 == 7)
            sys_sleep_interruptible(20);        // unanswered ARPs wait in small queues
    }
    for (int waited = 0; waited < 3000 && !signal_interrupted(); waited += 50)
        sys_sleep_interruptible(50);            // ARP retries take a second each
    icmp_set_observer(0);
    int up = 0;
    for (int i = 0; i < count; i++) {
        uint32_t host = net + (uint32_t)i;
        if (alive[(host & 0xFF) / 32] & (1u << ((host & 0xFF) % 32))) {
            char text[INET_ADDR_TEXT];
            inet_format(host, text);
            ui_printf("Nmap scan report for %s\nHost is up.\n", text);
            up++;
        }
    }
    ui_printf("Nmap done: %d IP addresses (%d hosts up) scanned\n", count, up);
}

/* ---------- The command ---------- */

void cmd_nmap(const char *args) {
    char word[DNS_NAME_MAX], host[DNS_NAME_MAX] = "", mode = 'T';
    int custom_ports = 0;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (word[0] == '-' && word[1] == 's' && word[2] && !word[3]) {
            mode = word[2];
        } else if (strcmp(word, "-p") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            if (!parse_ports(word)) {
                ui_usage(USAGE);
                return;
            }
            custom_ports = 1;
        } else {
            strcpy(host, word);
        }
    }
    if (!host[0] || (mode != 'T' && mode != 'S' && mode != 'U' && mode != 'n')) {
        ui_usage(USAGE);
        return;
    }
    if (mode != 'T' && !caps_require(CAP_NET_RAW, "nmap"))
        return;                                 // raw packets: root only
    if (!custom_ports) {
        port_count = sizeof(common_ports) / sizeof(common_ports[0]);
        for (int i = 0; i < port_count; i++)
            ports[i] = common_ports[i];
    }

    int prefix = 32;
    for (char *c = host; *c; c++)
        if (*c == '/') {
            *c = '\0';
            prefix = atoi(c + 1);
        }
    uint32_t ip;
    int rcode = dns_resolve(host, &ip);
    if (rcode != DNS_OK) {
        ui_error("nmap", "Failed to resolve the host");
        return;
    }
    ui_printf("Starting Nmap (UzOS) - %s scan\n", mode == 'T' ? "TCP connect" : mode == 'S' ? "SYN" :
              mode == 'U' ? "UDP" : "ping");
    if (mode == 'n') {
        ping_sweep(ip & inet_mask(prefix < 24 ? 24 : prefix), prefix);
        return;
    }

    uint64_t start = tsc_now();
    if (mode == 'T')
        connect_scan(ip);
    else
        raw_scan(ip, mode == 'U');

    char text[INET_ADDR_TEXT];
    inet_format(ip, text);
    ui_printf("Nmap scan report for %s (%s)\n", host, text);
    int hidden = 0;
    ui_color(UI_TITLE);
    ui_printf("PORT       STATE          SERVICE\n");
    ui_reset();
    for (int i = 0; i < port_count; i++) {
        port_state_t st = states[i];
        if (port_count > 25 && st != OPEN && st != OPEN_FILTERED) {
            hidden++;                           // a long list: show only the interesting ones
            continue;
        }
        char port_text[12];
        format(port_text, sizeof(port_text), "%u/%s", ports[i], mode == 'U' ? "udp" : "tcp");
        ui_printf("%-10s ", port_text);
        ui_color(st == OPEN ? UI_GOOD : st == CLOSED ? UI_DIM : UI_WARNING);
        ui_printf("%-14s ", st == OPEN ? "open" : st == CLOSED ? "closed" : st == FILTERED ? "filtered" : "open|filtered");
        ui_reset();
        ui_printf("%s\n", service(ports[i]));
    }
    if (hidden)
        ui_printf("Not shown: %d closed or filtered ports\n", hidden);
    uint32_t ms = tsc_cycles_to_us(tsc_now() - start) / 1000;
    ui_printf("Nmap done: 1 IP address (1 host up) scanned in %u.%02u seconds\n", ms / 1000, (ms % 1000) / 10);
}
