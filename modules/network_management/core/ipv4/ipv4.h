#ifndef IPV4_H
#define IPV4_H

/*
 * ipv4.h - IPv4: carry a packet from one address to another.
 *
 *   0       4       8              16                      31
 *   +-------+-------+--------------+------------------------+
 *   |version|  IHL  |     TOS      |      total length      |
 *   +-------+-------+--------------+---+--------------------+
 *   |        identification        |flg|  fragment offset   |
 *   +---------------+--------------+---+--------------------+
 *   |      TTL      |   protocol   |    header checksum     |
 *   +---------------+--------------+------------------------+
 *   |                     source address                    |
 *   |                  destination address                  |
 *   +-------------------------------------------------------+
 *   |   options (if IHL > 5)   |   data: ICMP / UDP / TCP   |
 *
 *   IHL       header length in 4-byte words (5 = 20 bytes, no options)
 *   TTL       "time to live": every router takes one off; at 0 the
 *             packet dies and the sender hears "time exceeded"
 *   protocol  1 = ICMP, 6 = TCP, 17 = UDP
 *   flags     DF = don't fragment, MF = more fragments follow
 *
 * A packet longer than the link's MTU is cut into FRAGMENTS that share
 * one identification; the receiver puts them back together (fragment.c).
 */

#include <stdint.h>
#include "modules/network_management/core/packet/packet.h"     // packet_t
#include "modules/network_management/core/netdev/netdev.h"     // netdev_t

#define IPV4_HLEN         20        // header without options
#define IPV4_DEFAULT_TTL  64        // what Linux starts with
#define IPPROTO_ICMP      1
#define IPPROTO_TCP       6
#define IPPROTO_UDP       17
#define IPV4_FLAG_DF      0x4000    // in 'frag', host order
#define IPV4_FLAG_MF      0x2000
#define IPV4_OFFSET_MASK  0x1FFF    // offset in 8-byte units
#define IPV4_BROADCAST    0xFFFFFFFF

typedef struct {
    uint8_t  version_ihl;           // 0x45 = version 4, 5 words
    uint8_t  tos;
    uint16_t total_length;          // header + data   (network byte order)
    uint16_t id;
    uint16_t frag;                  // flags + offset
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src;
    uint32_t dst;
} __attribute__((packed)) ipv4_header_t;

// Counters, as Linux's /proc/net/snmp names them (netstat -s).
typedef struct {
    uint32_t in_receives, in_hdr_errors, in_addr_errors, in_unknown_protos;
    uint32_t in_delivers, out_requests, out_no_routes;
    uint32_t reasm_reqds, reasm_oks, reasm_fails;
    uint32_t frag_oks, frag_fails, frag_creates;
} ipv4_stats_t;

// A received IP packet (Ethernet header pulled). Checks it, puts
// fragments together, hands the data to ICMP / UDP / TCP. Takes p.
void ipv4_receive(packet_t *p);

// Send p (its data = the ICMP / UDP / TCP header and payload) to dst.
// src 0 = our address on the way out; ttl 0 = IPV4_DEFAULT_TTL.
// Takes p. Returns 0, or a negative NET_E* error.
int ipv4_send(packet_t *p, uint32_t src, uint32_t dst, uint8_t protocol, uint8_t ttl);

// For the layers above: the IP header of a received packet (p->net_header).
static inline ipv4_header_t *ipv4_header(packet_t *p) {
    return (ipv4_header_t *)p->net_header;
}

ipv4_stats_t *ipv4_stats(void);

/* ---------- fragment.c ---------- */

#define IPV4_REASM_MAX     (PACKET_DATA_MAX - IPV4_HLEN)   // biggest datagram we rebuild
#define IPV4_REASM_SLOTS   4        // datagrams being rebuilt at once
#define IPV4_REASM_TIMEOUT 30000    // ms; then the pieces are thrown away

// A fragment arrived (header checked, still on p). Returns the whole
// datagram once every piece is in (a new packet, header in front), else
// NULL. Takes p.
packet_t *ipv4_reassemble(packet_t *p);

// Drop datagrams that never completed (net_tick).
void ipv4_fragment_tick(uint32_t now_ms);

// How many datagrams from 'src' (host order) are half built (seclab).
int ipv4_reassembly_count(uint32_t src);

// Throw every half-built datagram away (seclab's repair).
void ipv4_fragment_flush(void);

#endif
