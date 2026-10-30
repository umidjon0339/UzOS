/*
 * fragment.c - Put fragments back together into one datagram.
 *
 * The fragments of one datagram share (source, destination, id,
 * protocol). Each says where its bytes go ("offset", in 8-byte units),
 * and every one but the last has MF ("more fragments") set:
 *
 *   fragment 1   offset 0     MF   [ bytes    0 - 1479 ]
 *   fragment 2   offset 185   MF   [ bytes 1480 - 2959 ]
 *   fragment 3   offset 370        [ bytes 2960 - 3007 ]   <- the end is known now
 *
 * They may arrive in any order. Each datagram gets a slot with a
 * buffer and a bitmap of which 8-byte blocks are filled; when the last
 * fragment has said where the end is and every block before it is
 * filled, the datagram is complete.
 *
 * A fragment that would write past the buffer is refused: that check
 * is what the 1996 "ping of death" found missing (security/ping_of_death).
 *
 * Uses: packet_* (core/packet), ipv4_stats (ipv4.c), net_now_ms
 *       (core/net_tick), inet_checksum / htons (core/inet), memcpy / memset.
 */

#include "modules/network_management/core/ipv4/ipv4.h"           // this file's header
#include "modules/network_management/core/net_tick/net_tick.h"   // net_now_ms
#include "modules/network_management/core/inet/inet.h"           // inet_checksum, htons, ntohs
#include "lib/string/string.h"                                    // memcpy, memset
#include "modules/network_management/security/ping_of_death/defence/reassembly_bounds/def_reassembly_bounds.h" // def_reasm_fits
#include "modules/network_management/security/teardrop/defence/fragment_overlap_check/def_fragment_overlap_check.h" // def_frag_no_overlap
#include "modules/network_management/security/fragment_flood/defence/frag_mem_limit/def_frag_mem_limit.h" // def_frag_slot_ok

#define BLOCKS ((IPV4_REASM_MAX + 7) / 8)       // 8-byte blocks in one buffer

typedef struct {
    int used;
    uint32_t src, dst;                          // network order, as in the header
    uint16_t id;
    uint8_t protocol;
    uint16_t total;                             // data length; 0 = last fragment not seen
    uint32_t started_ms;
    ipv4_header_t header;                       // copied from a fragment, for the result
    uint8_t filled[(BLOCKS + 7) / 8];           // bit per 8-byte block
    uint8_t data[IPV4_REASM_MAX];
} reasm_t;

static reasm_t slots[IPV4_REASM_SLOTS];

static reasm_t *find_or_new(const ipv4_header_t *ip) {
    reasm_t *free_slot = 0;
    int held = 0;                               // slots this source already has
    for (int i = 0; i < IPV4_REASM_SLOTS; i++) {
        reasm_t *r = &slots[i];
        if (r->used && r->src == ip->src && r->dst == ip->dst && r->id == ip->id &&
            r->protocol == ip->protocol)
            return r;
        if (r->used && r->src == ip->src)
            held++;
        if (!r->used && !free_slot)
            free_slot = r;
    }
    // DEFENCE frag_mem_limit: a fair share of the slots per source.
    if (free_slot && def_frag_slot_ok(held)) {
        memset(free_slot->filled, 0, sizeof(free_slot->filled));
        free_slot->used = 1;
        free_slot->src = ip->src;
        free_slot->dst = ip->dst;
        free_slot->id = ip->id;
        free_slot->protocol = ip->protocol;
        free_slot->total = 0;
        free_slot->started_ms = net_now_ms();
        free_slot->header = *ip;
        return free_slot;
    }
    return 0;                                   // every slot busy, or this source's share used
}

static int complete(const reasm_t *r) {
    if (!r->total)
        return 0;
    for (int block = 0; block < (r->total + 7) / 8; block++)
        if (!(r->filled[block / 8] & (1 << (block % 8))))
            return 0;
    return 1;
}

static void give_up(reasm_t *r) {
    ipv4_stats()->reasm_fails++;
    r->used = 0;
}

packet_t *ipv4_reassemble(packet_t *p) {
    ipv4_header_t *ip = ipv4_header(p);
    uint16_t hlen = (uint16_t)((ip->version_ihl & 0x0F) * 4);
    uint16_t frag = ntohs(ip->frag);
    uint32_t offset = (uint32_t)(frag & IPV4_OFFSET_MASK) * 8;
    uint32_t len = p->len - hlen;
    int more = (frag & IPV4_FLAG_MF) != 0;

    reasm_t *r = find_or_new(ip);
    if (!r) {
        ipv4_stats()->reasm_fails++;            // too many half-built datagrams
        packet_free(p);
        return 0;
    }
    // Every piece but the last must be a whole number of 8-byte blocks.
    // DEFENCE reassembly_bounds: no piece may reach past our buffer (the
    // ping of death). DEFENCE fragment_overlap_check: nor cover bytes
    // another piece already brought (teardrop).
    if ((more && (len % 8)) || !def_reasm_fits(offset, len, IPV4_REASM_MAX) ||
        (len && !def_frag_no_overlap(r->filled, offset / 8, (offset + len - 1) / 8))) {
        give_up(r);
        packet_free(p);
        return 0;
    }
    memcpy(r->data + offset, p->data + hlen, len);
    for (uint32_t block = offset / 8; block < (offset + len + 7) / 8; block++)
        r->filled[block / 8] |= (uint8_t)(1 << (block % 8));
    if (!more)
        r->total = (uint16_t)(offset + len);
    if (offset == 0)
        r->header = *ip;                        // the first fragment's header leads
    netdev_t *dev = p->dev;
    packet_free(p);

    if (!complete(r))
        return 0;

    // All in: one packet, a plain 20-byte header, then the data.
    packet_t *whole = packet_alloc();
    if (!whole) {
        give_up(r);
        return 0;
    }
    memcpy(packet_put(whole, r->total), r->data, r->total);
    ipv4_header_t *h = (ipv4_header_t *)packet_push(whole, IPV4_HLEN);
    *h = r->header;
    h->version_ihl = 0x45;
    h->total_length = htons((uint16_t)(IPV4_HLEN + r->total));
    h->frag = 0;
    h->checksum = 0;
    h->checksum = inet_checksum(h, IPV4_HLEN);
    whole->dev = dev;                           // where the last piece came in
    whole->net_header = whole->data;
    r->used = 0;
    ipv4_stats()->reasm_oks++;
    return whole;
}

int ipv4_reassembly_count(uint32_t src) {
    int n = 0;
    for (int i = 0; i < IPV4_REASM_SLOTS; i++)
        n += slots[i].used && ntohl(slots[i].src) == src;
    return n;
}

void ipv4_fragment_flush(void) {
    for (int i = 0; i < IPV4_REASM_SLOTS; i++)
        slots[i].used = 0;
}

void ipv4_fragment_tick(uint32_t now_ms) {
    for (int i = 0; i < IPV4_REASM_SLOTS; i++)
        if (slots[i].used && now_ms - slots[i].started_ms >= IPV4_REASM_TIMEOUT)
            give_up(&slots[i]);                 // a piece never came
}
