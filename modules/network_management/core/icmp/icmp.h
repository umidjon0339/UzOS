#ifndef ICMP_H
#define ICMP_H

/*
 * icmp.h - ICMP: IP's messages about itself.
 *
 *   0        8        16                31
 *   +--------+--------+-----------------+
 *   |  type  |  code  |    checksum     |
 *   +--------+--------+-----------------+
 *   |  id (echo)      |   seq (echo)    |   or "unused" in errors
 *   +-----------------+-----------------+
 *   |  echo: any data | error: the IP header + 8 bytes of the packet
 *                     |        that caused it
 *
 *   type 8  echo request     "are you there?"         (ping sends)
 *   type 0  echo reply       "yes"                    (ping waits for)
 *   type 3  destination unreachable  code 1 host, 2 protocol, 3 port
 *   type 11 time exceeded    TTL reached 0 on the way (traceroute)
 *
 * Replies and errors are passed to an observer: ping and traceroute
 * set one while they run.
 */

#include <stdint.h>
#include "modules/network_management/core/packet/packet.h"     // packet_t

#define ICMP_HLEN            8
#define ICMP_ECHO_REPLY      0
#define ICMP_DEST_UNREACH    3
#define ICMP_REDIRECT        5
#define ICMP_ECHO_REQUEST    8
#define ICMP_TIME_EXCEEDED   11

// Codes of ICMP_DEST_UNREACH
#define ICMP_NET_UNREACH     0
#define ICMP_HOST_UNREACH    1
#define ICMP_PROTO_UNREACH   2
#define ICMP_PORT_UNREACH    3
#define ICMP_FRAG_NEEDED     4
// Codes of ICMP_TIME_EXCEEDED
#define ICMP_TTL_EXCEEDED    0
#define ICMP_REASM_EXCEEDED  1

typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t id;                    // echo: who is asking   (network byte order)
    uint16_t seq;                   // echo: which question
} __attribute__((packed)) icmp_header_t;

// What the observer is told.
typedef struct {
    uint8_t  type, code;
    uint32_t from;                  // who sent the ICMP message (host order)
    uint8_t  ttl;                   // the TTL it arrived with
    uint16_t id, seq;               // echo reply: its own; an error about our
                                    //   echo request: that request's (host order)
    uint16_t data_len;              // echo reply: bytes after the ICMP header
    const uint8_t *data;
} icmp_event_t;

typedef void (*icmp_observer_t)(const icmp_event_t *event);

typedef struct {
    uint32_t in_msgs, in_errors, in_echos, in_echo_reps, in_dest_unreachs, in_time_excds;
    uint32_t out_msgs, out_echos, out_echo_reps, out_dest_unreachs;
} icmp_stats_t;

// A received ICMP message (IP header pulled; p->net_header still points
// at it). Answers echo requests; tells the observer the rest. Takes p.
void icmp_receive(packet_t *p);

// Send an echo request ("ping") to dst. Returns 0 or a negative NET_E*.
int icmp_send_echo(uint32_t dst, uint16_t id, uint16_t seq, uint8_t ttl,
                   const void *data, uint16_t len);

// Tell the sender of 'cause' that something went wrong (type, code).
// 'cause' must be a received packet: its net_header is quoted. Does not
// take 'cause'.
void icmp_send_error(packet_t *cause, uint8_t type, uint8_t code);

void icmp_set_observer(icmp_observer_t fn);
icmp_stats_t *icmp_stats(void);

#endif
