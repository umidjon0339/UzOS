/*
 * ethernet.c - Check a received frame and pass it up; frame what goes out.
 *
 * Receive filter (the card already did this, but promiscuous mode turns
 * the card's filter off): keep frames for our MAC or for everyone
 * (broadcast). Frames for other machines are only for tcpdump.
 *
 * Uses: packet_* (core/packet), netdev_transmit (core/netdev),
 *       arp_receive (core/arp), ipv4_receive (core/ipv4), ntohs / htons (core/inet), memcpy (lib/string).
 */

#include "modules/network_management/core/ethernet/ethernet.h"   // this file
#include "modules/network_management/core/inet/inet.h"           // ntohs, htons
#include "modules/network_management/core/arp/arp.h"             // arp_receive
#include "modules/network_management/core/ipv4/ipv4.h"           // ipv4_receive
#include "lib/string/string.h"                                    // memcpy
#include "modules/network_management/security/malformed_frame/defence/frame_validate/def_frame_validate.h"  // def_frame_ok

const uint8_t ETH_BROADCAST[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static uint32_t unknown;

static int same_mac(const uint8_t *a, const uint8_t *b) {
    for (int i = 0; i < ETH_ALEN; i++)
        if (a[i] != b[i])
            return 0;
    return 1;
}

void ethernet_receive(packet_t *p) {
    p->mac_header = p->data;
    uint32_t len = p->len;
    eth_header_t *eth = (eth_header_t *)packet_pull(p, ETH_HLEN);
    // DEFENCE frame_validate: 14-1514 bytes and a real type, or nothing.
    if (!eth || !def_frame_ok(len, ntohs(eth->type))) {
        p->dev->stats.rx_errors++;              // shorter than a header
        packet_free(p);
        return;
    }

    // Not for us (only seen in promiscuous mode): nothing to do here.
    if (!same_mac(eth->dest, p->dev->mac) && !same_mac(eth->dest, ETH_BROADCAST) &&
        !(p->dev->flags & NETDEV_LOOPBACK)) {
        packet_free(p);
        return;
    }

    switch (ntohs(eth->type)) {
    case ETH_P_ARP:
        arp_receive(p);
        break;
    case ETH_P_IP:
        ipv4_receive(p);
        break;
    default:
        unknown++;                              // nobody speaks this protocol
        packet_free(p);
    }
}

int ethernet_send(netdev_t *dev, const uint8_t dest[ETH_ALEN], uint16_t type, packet_t *p) {
    eth_header_t *eth = (eth_header_t *)packet_push(p, ETH_HLEN);
    if (!eth) {
        packet_free(p);
        return 0;
    }
    memcpy(eth->dest, dest, ETH_ALEN);
    memcpy(eth->src, dev->mac, ETH_ALEN);
    eth->type = htons(type);
    return netdev_transmit(dev, p);
}

uint32_t ethernet_unknown_count(void) {
    return unknown;
}
