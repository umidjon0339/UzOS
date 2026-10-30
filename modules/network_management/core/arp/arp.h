#ifndef ARP_H
#define ARP_H

/*
 * arp.h - ARP: find the MAC address that belongs to an IP address.
 *
 * IP says "send to 10.0.2.2", but an Ethernet frame needs a MAC. So we
 * shout to everyone on the link (broadcast):
 *
 *   request  "who has 10.0.2.2? tell 10.0.2.15 (52:54:00:12:34:56)"
 *   reply    "10.0.2.2 is at 52:55:0a:00:02:02"         (only to us)
 *
 * The answer goes in the ARP cache (`arp`, `ip neigh`), so the next
 * packet goes at once. While a request is out, packets for that address
 * wait in its entry; if nobody answers after ARP_RETRIES tries, they
 * are dropped.
 *
 *   ARP packet (28 bytes, after the Ethernet header, type 0x0806):
 *   htype=1 | ptype=0x0800 | hlen=6 | plen=4 | op=1/2 |
 *   sender MAC | sender IP | target MAC | target IP
 *
 * ARP believes whatever it hears: nothing proves a reply is honest.
 * That is the ARP spoofing attack (security/arp_spoofing).
 */

#include <stdint.h>
#include "modules/network_management/core/packet/packet.h"     // packet_t
#include "modules/network_management/core/netdev/netdev.h"     // netdev_t

#define ARP_CACHE_SIZE  32
#define ARP_QUEUE_MAX   3           // packets that may wait for one answer
#define ARP_RETRIES     3           // requests before we give up
#define ARP_RETRY_MS    1000
#define ARP_REACHABLE_MS 60000      // a learned entry is trusted this long

#define ARP_OP_REQUEST  1
#define ARP_OP_REPLY    2

typedef struct {
    uint16_t htype;                 // 1 = Ethernet        (network byte order)
    uint16_t ptype;                 // 0x0800 = IPv4
    uint8_t  hlen;                  // 6
    uint8_t  plen;                  // 4
    uint16_t op;                    // ARP_OP_*
    uint8_t  sender_mac[6];
    uint32_t sender_ip;             // network byte order
    uint8_t  target_mac[6];
    uint32_t target_ip;
} __attribute__((packed)) arp_packet_t;

typedef enum {
    ARP_FREE,                       // unused slot
    ARP_INCOMPLETE,                 // asked, no answer yet
    ARP_REACHABLE,                  // answered, recently
    ARP_PERMANENT,                  // added by hand (arp -s): never ages, never overwritten
} arp_state_t;

typedef struct {
    uint32_t ip;                    // host order
    uint8_t mac[6];
    netdev_t *dev;
    arp_state_t state;
    uint32_t updated_ms;            // learned, or last request sent
    int tries;                      // requests sent while INCOMPLETE
    int unasked;                    // learned from a request, not from our own question
    packet_queue_t waiting;         // packets for this address, until it answers
} arp_entry_t;

typedef struct {
    uint32_t requests_sent, requests_received;
    uint32_t replies_sent, replies_received;
    uint32_t unresolved;            // gave up: nobody answered
} arp_stats_t;

// A received ARP packet (Ethernet header already pulled). Takes it.
void arp_receive(packet_t *p);

// Send IP packet p to 'next_hop' on dev: at once if its MAC is known,
// else after the answer. Takes the packet. Returns 1 if sent or queued.
int arp_output(netdev_t *dev, uint32_t next_hop, packet_t *p);

// Broadcast "who has ip?" on dev.
void arp_request(netdev_t *dev, uint32_t ip);

// Retries and ageing (net_tick).
void arp_tick(uint32_t now_ms);

// The table, for arp / ip neigh.
const arp_entry_t *arp_entry(int slot);       // slot 0 .. ARP_CACHE_SIZE-1
int  arp_lookup(uint32_t ip, uint8_t mac[6]);  // 1 if known
int  arp_add_static(netdev_t *dev, uint32_t ip, const uint8_t mac[6]);
int  arp_delete(uint32_t ip);
void arp_flush(netdev_t *dev);                 // forget every learned entry on dev
const arp_stats_t *arp_stats(void);

// Told about every reply that arrives (arping measures the time).
typedef void (*arp_observer_t)(uint32_t ip, const uint8_t mac[6]);
void arp_set_observer(arp_observer_t fn);

#endif
