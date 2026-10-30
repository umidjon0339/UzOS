/*
 * dhcp.c - The DHCP conversation: DISCOVER, OFFER, REQUEST, ACK.
 *
 * A DHCP message (RFC 2131) is the old BOOTP layout plus "options":
 *
 *   op  htype hlen hops | xid | secs flags | ciaddr | yiaddr | siaddr |
 *   giaddr | chaddr (our MAC) | sname | file | 63 82 53 63 | options...
 *
 *   yiaddr  "your address": what the server gives us
 *   options [code, length, value...]: 53 message type, 1 netmask,
 *           3 router, 6 DNS server, 51 lease time, 54 server id, 255 end
 *
 * Uses: udp_bind / udp_send (core/udp), netdev_set_address (core/netdev),
 *       route_add (core/route), net_now_ms (core/net_tick), tsc_now
 *       (cpu_management, for a random xid), htonl / htons (core/inet),
 *       memset / memcpy (lib/string).
 */

#include "modules/network_management/core/dhcp/dhcp.h"           // this file
#include "modules/network_management/core/udp/udp.h"             // udp_bind, udp_send
#include "modules/network_management/core/route/route.h"         // route_add
#include "modules/network_management/core/ipv4/ipv4.h"           // IPV4_BROADCAST
#include "modules/network_management/core/net_tick/net_tick.h"   // net_now_ms
#include "modules/network_management/core/inet/inet.h"           // htonl, ntohl, htons
#include "modules/cpu_management/core/tsc/tsc.h"                 // tsc_now
#include "lib/string/string.h"                                    // memset, memcpy
#include "modules/network_management/security/dhcp_spoofing/defence/dhcp_validate/def_dhcp_validate.h"  // def_dhcp_offer_ok

#define BOOTREQUEST   1
#define BOOTREPLY     2
#define MAGIC_COOKIE  0x63825363
#define FLAG_BROADCAST 0x8000       // "answer by broadcast: I have no address yet"

// Option codes
#define OPT_PAD        0
#define OPT_NETMASK    1
#define OPT_ROUTER     3
#define OPT_DNS        6
#define OPT_HOSTNAME   12
#define OPT_REQUESTED  50
#define OPT_LEASE      51
#define OPT_MSG_TYPE   53
#define OPT_SERVER_ID  54
#define OPT_PARAMS     55
#define OPT_END        255

typedef struct {
    uint8_t  op, htype, hlen, hops;
    uint32_t xid;
    uint16_t secs, flags;
    uint32_t ciaddr, yiaddr, siaddr, giaddr;    // network byte order
    uint8_t  chaddr[16];
    uint8_t  sname[64];
    uint8_t  file[128];
    uint32_t cookie;
    uint8_t  options[64];
} __attribute__((packed)) dhcp_message_t;       // 300 bytes: BOOTP's minimum

static dhcp_lease_t lease;
static uint32_t known_server;           // who gave us our last lease (outlives dhcp_start)
static udp_endpoint_t endpoint;
static dhcp_event_t history[DHCP_LOG_SIZE];
static int log_count;

static void note(uint8_t type, uint32_t ip, uint32_t server) {
    dhcp_event_t *e = &history[log_count % DHCP_LOG_SIZE];
    e->type = type;
    e->ip = ip;
    e->server = server;
    log_count++;
}

int dhcp_log_count(void) {
    return log_count;
}

const dhcp_event_t *dhcp_log_get(int n) {
    if (n < 0 || n >= log_count || n < log_count - DHCP_LOG_SIZE)
        return 0;
    return &history[n % DHCP_LOG_SIZE];
}

const dhcp_lease_t *dhcp_lease(void) {
    return &lease;
}

const char *dhcp_state_name(dhcp_state_t state) {
    static const char *names[] = { "off", "selecting", "requesting", "bound", "failed" };
    return names[state];
}

/* ---------- Sending ---------- */

// Fill the fixed part; returns where the options go.
static uint8_t *begin(dhcp_message_t *m, uint8_t type) {
    memset(m, 0, sizeof(*m));
    m->op = BOOTREQUEST;
    m->htype = 1;                               // Ethernet
    m->hlen = 6;
    m->xid = htonl(lease.xid);
    m->flags = htons(FLAG_BROADCAST);
    memcpy(m->chaddr, lease.dev->mac, 6);
    m->cookie = htonl(MAGIC_COOKIE);
    uint8_t *o = m->options;
    *o++ = OPT_MSG_TYPE; *o++ = 1; *o++ = type;
    return o;
}

static uint8_t *put_ip(uint8_t *o, uint8_t code, uint32_t ip) {
    *o++ = code;
    *o++ = 4;
    uint32_t net = htonl(ip);
    memcpy(o, &net, 4);
    return o + 4;
}

static void send_message(dhcp_message_t *m, uint8_t type, uint32_t to) {
    note(type, lease.ip, lease.server);
    lease.sent_ms = net_now_ms();
    lease.tries++;
    udp_send(0, DHCP_CLIENT_PORT, to, DHCP_SERVER_PORT, m, sizeof(*m), 0);
}

static void send_discover(void) {
    dhcp_message_t m;
    uint8_t *o = begin(&m, DHCPDISCOVER);
    *o++ = OPT_PARAMS; *o++ = 3; *o++ = OPT_NETMASK; *o++ = OPT_ROUTER; *o++ = OPT_DNS;
    *o = OPT_END;
    send_message(&m, DHCPDISCOVER, IPV4_BROADCAST);
}

static void send_request(void) {
    dhcp_message_t m;
    uint8_t *o = begin(&m, DHCPREQUEST);
    o = put_ip(o, OPT_REQUESTED, lease.ip);
    o = put_ip(o, OPT_SERVER_ID, lease.server);
    *o++ = OPT_PARAMS; *o++ = 3; *o++ = OPT_NETMASK; *o++ = OPT_ROUTER; *o++ = OPT_DNS;
    *o = OPT_END;
    send_message(&m, DHCPREQUEST, IPV4_BROADCAST);  // broadcast: other servers learn we chose
}

/* ---------- Receiving ---------- */

static int is_our_mac(const uint8_t *mac) {
    for (int i = 0; i < 6; i++)
        if (mac[i] != lease.dev->mac[i])
            return 0;
    return 1;
}

// Read the options we care about into 'out'. Returns the message type.
static int read_options(const dhcp_message_t *m, uint16_t len, dhcp_lease_t *out) {
    const uint8_t *o = m->options;
    const uint8_t *end = (const uint8_t *)m + len;
    int type = 0;
    while (o < end && *o != OPT_END) {
        if (*o == OPT_PAD) {
            o++;
            continue;
        }
        if (o + 2 > end || o + 2 + o[1] > end)
            break;                              // an option runs past the end
        uint8_t code = o[0], size = o[1];
        const uint8_t *value = o + 2;
        uint32_t ip = size >= 4 ? ntohl(*(const uint32_t *)value) : 0;
        if (code == OPT_MSG_TYPE && size >= 1) type = value[0];
        if (code == OPT_NETMASK && size >= 4)  out->netmask = ip;
        if (code == OPT_ROUTER && size >= 4)   out->router = ip;     // the first router
        if (code == OPT_DNS && size >= 4)      out->dns = ip;        // the first DNS server
        if (code == OPT_LEASE && size >= 4)    out->lease_s = ip;
        if (code == OPT_SERVER_ID && size >= 4) out->server = ip;
        o += 2 + size;
    }
    return type;
}

static void bind_lease(void) {
    netdev_t *dev = lease.dev;
    netdev_set_address(dev, lease.ip, lease.netmask);   // also adds 10.0.2.0/24
    if (lease.router)
        route_add(0, 0, lease.router, dev, 0, ROUTE_DHCP);
    lease.state = DHCP_BOUND;
    known_server = lease.server;
    lease.bound_ms = net_now_ms();
    lease.tries = 0;
}

static void on_message(udp_endpoint_t *ep, packet_t *p, uint32_t src, uint16_t sport) {
    (void)ep;
    (void)src;
    (void)sport;
    const dhcp_message_t *m = (const dhcp_message_t *)p->data;
    if (p->len < sizeof(*m) - sizeof(m->options) || m->op != BOOTREPLY ||
        ntohl(m->xid) != lease.xid || ntohl(m->cookie) != MAGIC_COOKIE ||
        !lease.dev || !is_our_mac(m->chaddr)) {
        packet_free(p);                         // not an answer to our question
        return;
    }
    dhcp_lease_t offer = lease;
    offer.netmask = offer.router = offer.dns = offer.server = 0;
    int type = read_options(m, p->len, &offer);
    offer.ip = ntohl(m->yiaddr);
    if (type == DHCPOFFER || type == DHCPACK || type == DHCPNAK)
        note((uint8_t)type, offer.ip, offer.server);

    if (lease.state == DHCP_SELECTING && type == DHCPOFFER &&
        def_dhcp_offer_ok(offer.ip, offer.netmask, offer.router, offer.server, known_server)) {
        // DEFENCE dhcp_validate: a sane offer, from the server we know.
        lease = offer;                          // the first offer wins
        lease.state = DHCP_REQUESTING;
        lease.tries = 0;
        send_request();
    } else if (lease.state == DHCP_REQUESTING && type == DHCPACK && offer.server == lease.server) {
        lease = offer;
        bind_lease();
    } else if (lease.state == DHCP_REQUESTING && type == DHCPNAK) {
        lease.state = DHCP_FAILED;              // the server said no
    }
    packet_free(p);
}

/* ---------- Starting and stopping ---------- */

void dhcp_start(netdev_t *dev) {
    if (!endpoint.receive) {
        endpoint.local_port = DHCP_CLIENT_PORT;
        endpoint.receive = on_message;
        if (udp_bind(&endpoint) < 0)
            endpoint.receive = 0;
    }
    memset(&lease, 0, sizeof(lease));
    lease.dev = dev;
    lease.xid = (uint32_t)tsc_now() ^ 0x5A17D0C5;   // anything others cannot guess easily
    lease.state = DHCP_SELECTING;
    netdev_set_address(dev, 0, 0);              // no address while asking
    send_discover();
}

void dhcp_release(void) {
    if (lease.state != DHCP_BOUND)
        return;
    dhcp_message_t m;
    uint8_t *o = begin(&m, DHCPRELEASE);
    m.flags = 0;
    m.ciaddr = htonl(lease.ip);
    o = put_ip(o, OPT_SERVER_ID, lease.server);
    *o = OPT_END;
    send_message(&m, DHCPRELEASE, lease.server);    // to the server itself
    netdev_set_address(lease.dev, 0, 0);
    lease.state = DHCP_OFF;
}

void dhcp_tick(uint32_t now_ms) {
    if ((lease.state != DHCP_SELECTING && lease.state != DHCP_REQUESTING) ||
        now_ms - lease.sent_ms < DHCP_RETRY_MS)
        return;
    if (lease.tries >= DHCP_RETRIES) {
        lease.state = DHCP_FAILED;              // nobody answers
    } else if (lease.state == DHCP_SELECTING) {
        send_discover();
    } else {
        send_request();
    }
}
