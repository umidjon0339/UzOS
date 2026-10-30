/*
 * arp.c - The ARP cache: ask, answer, learn, forget.
 *
 * Learning follows RFC 826 ("merge"): an ARP packet from an address we
 * already know updates its MAC; a request aimed at us adds the asker,
 * since we are about to talk to it anyway.
 *
 * Two defences decide what may be learned: arp_guard (no packet may
 * change a known MAC) and arp_table_limit (few entries from strangers).
 *
 * Uses: packet_* (core/packet), ethernet_send / ETH_BROADCAST
 *       (core/ethernet), net_now_ms (core/net_tick), htons / ntohl
 *       (core/inet), memcpy (lib/string).
 */

#include "modules/network_management/core/arp/arp.h"             // this file
#include "modules/network_management/core/ethernet/ethernet.h"   // ethernet_send, ETH_BROADCAST, ETH_P_*
#include "modules/network_management/core/net_tick/net_tick.h"   // net_now_ms
#include "modules/network_management/core/inet/inet.h"           // htons, ntohs, htonl, ntohl
#include "lib/string/string.h"                                    // memcpy, memset
#include "modules/network_management/security/arp_spoofing/defence/arp_guard/def_arp_guard.h"        // def_arp_guard_ok
#include "modules/network_management/security/arp_flood/defence/arp_table_limit/def_arp_table_limit.h" // def_arp_learn_ok

#define HTYPE_ETHERNET 1

static arp_entry_t cache[ARP_CACHE_SIZE];
static arp_stats_t stats;
static arp_observer_t observer;

/* ---------- The cache ---------- */

static int same_mac(const uint8_t *a, const uint8_t *b) {
    for (int i = 0; i < 6; i++)
        if (a[i] != b[i])
            return 0;
    return 1;
}

// Entries learned from requests nobody asked us to send.
static int count_unasked(void) {
    int n = 0;
    for (int i = 0; i < ARP_CACHE_SIZE; i++)
        n += cache[i].state != ARP_FREE && cache[i].unasked;
    return n;
}

static arp_entry_t *find(uint32_t ip) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++)
        if (cache[i].state != ARP_FREE && cache[i].ip == ip)
            return &cache[i];
    return 0;
}

static void release(arp_entry_t *e) {
    packet_queue_flush(&e->waiting);
    memset(e, 0, sizeof(*e));           // state = ARP_FREE
}

// A free slot; if the table is full, the oldest learned entry makes room.
static arp_entry_t *new_entry(uint32_t ip, netdev_t *dev) {
    arp_entry_t *slot = 0;
    for (int i = 0; i < ARP_CACHE_SIZE && !slot; i++)
        if (cache[i].state == ARP_FREE)
            slot = &cache[i];
    if (!slot) {
        for (int i = 0; i < ARP_CACHE_SIZE; i++)
            if (cache[i].state == ARP_REACHABLE &&
                (!slot || cache[i].updated_ms < slot->updated_ms))
                slot = &cache[i];
    }
    if (!slot)
        return 0;
    release(slot);
    slot->ip = ip;
    slot->dev = dev;
    return slot;
}

// The answer is in: remember it, and send everything that was waiting.
static void resolved(arp_entry_t *e, const uint8_t mac[6]) {
    memcpy(e->mac, mac, 6);
    e->state = ARP_REACHABLE;
    e->updated_ms = net_now_ms();
    e->tries = 0;
    packet_t *p;
    while ((p = packet_queue_pop(&e->waiting)))
        ethernet_send(e->dev, e->mac, ETH_P_IP, p);
}

/* ---------- Sending ARP packets ---------- */

static void send_arp(netdev_t *dev, uint16_t op, const uint8_t dest_mac[6],
                     const uint8_t target_mac[6], uint32_t target_ip) {
    packet_t *p = packet_alloc();
    if (!p)
        return;
    arp_packet_t *a = (arp_packet_t *)packet_put(p, sizeof(arp_packet_t));
    a->htype = htons(HTYPE_ETHERNET);
    a->ptype = htons(ETH_P_IP);
    a->hlen = 6;
    a->plen = 4;
    a->op = htons(op);
    memcpy(a->sender_mac, dev->mac, 6);
    a->sender_ip = htonl(dev->ip);
    memcpy(a->target_mac, target_mac, 6);
    a->target_ip = htonl(target_ip);
    // Ethernet's shortest frame is 60 bytes: pad the 42 we have.
    memset(packet_put(p, ETH_ZLEN - ETH_HLEN - sizeof(arp_packet_t)), 0,
           ETH_ZLEN - ETH_HLEN - sizeof(arp_packet_t));
    ethernet_send(dev, dest_mac, ETH_P_ARP, p);
}

void arp_request(netdev_t *dev, uint32_t ip) {
    static const uint8_t unknown[6] = { 0 };
    // Remember that we asked: arp_guard accepts only answers to questions.
    if (!find(ip)) {
        arp_entry_t *e = new_entry(ip, dev);
        if (e) {
            e->state = ARP_INCOMPLETE;
            e->updated_ms = net_now_ms();
            e->tries = 1;
        }
    }
    stats.requests_sent++;
    send_arp(dev, ARP_OP_REQUEST, ETH_BROADCAST, unknown, ip);
}

/* ---------- Receiving ---------- */

void arp_receive(packet_t *p) {
    netdev_t *dev = p->dev;
    arp_packet_t *a = (arp_packet_t *)packet_pull(p, sizeof(arp_packet_t));
    if (!a || ntohs(a->htype) != HTYPE_ETHERNET || ntohs(a->ptype) != ETH_P_IP ||
        a->hlen != 6 || a->plen != 4) {
        dev->stats.rx_errors++;                 // not Ethernet/IPv4 ARP
        packet_free(p);
        return;
    }
    uint16_t op = ntohs(a->op);
    uint32_t sender = ntohl(a->sender_ip), target = ntohl(a->target_ip);
    int for_us = dev->ip && target == dev->ip;

    // Learn the sender: DEFENCE arp_guard decides whether this packet
    // may set (or change) the sender's MAC at all.
    arp_entry_t *e = find(sender);
    if (sender && def_arp_guard_ok(e ? (int)e->state : ARP_FREE,
                                   e && same_mac(e->mac, a->sender_mac),
                                   op == ARP_OP_REQUEST && for_us)) {
        // DEFENCE arp_table_limit: only a few entries from strangers.
        if (!e && def_arp_learn_ok(count_unasked()) && (e = new_entry(sender, dev)))
            e->unasked = 1;
        if (e)
            resolved(e, a->sender_mac);
    }

    if (op == ARP_OP_REQUEST) {
        stats.requests_received++;
        if (for_us) {                           // "who has <us>?" -> "we are at <our MAC>"
            stats.replies_sent++;
            send_arp(dev, ARP_OP_REPLY, a->sender_mac, a->sender_mac, sender);
        }
    } else if (op == ARP_OP_REPLY) {
        stats.replies_received++;
        if (observer)
            observer(sender, a->sender_mac);
    }
    packet_free(p);
}

/* ---------- Sending IP packets ---------- */

int arp_output(netdev_t *dev, uint32_t next_hop, packet_t *p) {
    arp_entry_t *e = find(next_hop);
    if (e && e->state != ARP_INCOMPLETE)
        return ethernet_send(dev, e->mac, ETH_P_IP, p);

    if (!e) {
        e = new_entry(next_hop, dev);
        if (!e) {
            packet_free(p);                     // table full of hand-made entries
            return 0;
        }
        e->state = ARP_INCOMPLETE;
        e->updated_ms = net_now_ms();
        e->tries = 1;
        arp_request(dev, next_hop);
    }
    if (e->waiting.count >= ARP_QUEUE_MAX) {
        packet_free(p);                         // enough already waiting
        return 0;
    }
    packet_queue_push(&e->waiting, p);
    return 1;
}

/* ---------- Timers ---------- */

void arp_tick(uint32_t now_ms) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        arp_entry_t *e = &cache[i];
        uint32_t age = now_ms - e->updated_ms;
        if (e->state == ARP_INCOMPLETE && age >= ARP_RETRY_MS) {
            if (e->tries >= ARP_RETRIES) {
                stats.unresolved++;             // nobody home: drop what waited
                release(e);
            } else {
                e->tries++;
                e->updated_ms = now_ms;
                arp_request(e->dev, e->ip);
            }
        } else if (e->state == ARP_REACHABLE && age >= ARP_REACHABLE_MS) {
            release(e);                         // old news: ask again next time
        }
    }
}

/* ---------- The table, for commands ---------- */

const arp_entry_t *arp_entry(int slot) {
    return slot >= 0 && slot < ARP_CACHE_SIZE ? &cache[slot] : 0;
}

int arp_lookup(uint32_t ip, uint8_t mac[6]) {
    arp_entry_t *e = find(ip);
    if (!e || e->state == ARP_INCOMPLETE)
        return 0;
    memcpy(mac, e->mac, 6);
    return 1;
}

int arp_add_static(netdev_t *dev, uint32_t ip, const uint8_t mac[6]) {
    arp_entry_t *e = find(ip);
    if (!e)
        e = new_entry(ip, dev);
    if (!e)
        return 0;
    resolved(e, mac);
    e->dev = dev;
    e->state = ARP_PERMANENT;
    return 1;
}

int arp_delete(uint32_t ip) {
    arp_entry_t *e = find(ip);
    if (!e)
        return 0;
    release(e);
    return 1;
}

void arp_flush(netdev_t *dev) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++)
        if (cache[i].state != ARP_FREE && cache[i].state != ARP_PERMANENT &&
            (!dev || cache[i].dev == dev))
            release(&cache[i]);
}

const arp_stats_t *arp_stats(void) {
    return &stats;
}

void arp_set_observer(arp_observer_t fn) {
    observer = fn;
}
