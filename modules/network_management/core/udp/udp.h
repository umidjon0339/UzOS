#ifndef UDP_H
#define UDP_H

/*
 * udp.h - UDP: datagrams to a PORT on a machine.
 *
 *   0               16              31
 *   +---------------+---------------+
 *   |  source port  |  dest port    |
 *   +---------------+---------------+
 *   |    length     |   checksum    |    length = header + data
 *   +---------------+---------------+
 *   |             data              |
 *
 * IP brings a packet to a machine; the port says which program gets it
 * (53 = DNS, 67/68 = DHCP, ...). UDP adds nothing else: no connection,
 * no order, no resending. A datagram arrives whole or not at all.
 *
 * A program (a socket, or the kernel's own DHCP client) owns a port
 * through an ENDPOINT: it fills one in and udp_bind()s it. Datagrams for
 * that port go to its receive() function; ICMP "port unreachable" about
 * what it sent goes to its error() function.
 */

#include <stdint.h>
#include "modules/network_management/core/packet/packet.h"     // packet_t

#define UDP_HLEN           8
#define UDP_MAX_ENDPOINTS  32
#define UDP_EPHEMERAL_LOW  32768    // ports handed out by bind(port 0), as Linux
#define UDP_EPHEMERAL_HIGH 60999

typedef struct {
    uint16_t src_port;              // network byte order
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;              // 0 = the sender did not compute one
} __attribute__((packed)) udp_header_t;

typedef struct udp_endpoint {
    uint32_t local_ip;              // host order; 0 = any of our addresses
    uint16_t local_port;            // host order; 0 in udp_bind() = pick one
    uint32_t remote_ip;             // connected: only from here (0 = anyone)
    uint16_t remote_port;
    uint32_t uid;                   // who owns it
    int reuseport;                  // SO_REUSEPORT: may share the port (same owner only)
    // A datagram arrived (p->data = the payload; src/sport host order). Takes p.
    void (*receive)(struct udp_endpoint *ep, packet_t *p, uint32_t src, uint16_t sport);
    // What we sent was refused (-NET_ECONNREFUSED). May be NULL.
    void (*error)(struct udp_endpoint *ep, int err);
    void *owner;                    // the socket (or NULL for the kernel)
} udp_endpoint_t;

typedef struct {
    uint32_t in_datagrams, no_ports, in_errors, out_datagrams, rcvbuf_errors;
} udp_stats_t;

// Take ep->local_port (or a free ephemeral port if 0). 0 or -NET_EADDRINUSE.
int udp_bind(udp_endpoint_t *ep);
void udp_unbind(udp_endpoint_t *ep);

// Send 'len' bytes from (src, sport) to (dst, dport); src 0 = pick by
// route, ttl 0 = the default. 0 or a negative NET_E*.
int udp_send(uint32_t src, uint16_t sport, uint32_t dst, uint16_t dport,
             const void *data, uint16_t len, uint8_t ttl);

// A received UDP packet (IP header pulled). Takes p.
void udp_receive(packet_t *p);

// ICMP said our datagram was refused: 'quote' is our IP header + 8 bytes.
void udp_icmp_error(const uint8_t *quote, uint16_t len, int err);

const udp_endpoint_t *udp_endpoint(int slot);   // for netstat / ss
udp_stats_t *udp_stats(void);

#endif
