#ifndef ETHERNET_H
#define ETHERNET_H

/*
 * ethernet.h - Ethernet frames: who it is for, who sent it, what is inside.
 *
 *   0      6      12   14
 *   +------+------+----+---------------------------+
 *   | dest | src  |type| payload (46-1500 bytes)   |
 *   +------+------+----+---------------------------+
 *     MAC    MAC   0x0800 = IPv4, 0x0806 = ARP
 *
 * The card adds a preamble before and a CRC after; we never see those.
 */

#include <stdint.h>
#include "modules/network_management/core/packet/packet.h"     // packet_t
#include "modules/network_management/core/netdev/netdev.h"     // netdev_t

#define ETH_ALEN    6           // bytes in a MAC address
#define ETH_HLEN    14          // bytes in the header
#define ETH_ZLEN    60          // shortest frame (without CRC): pad up to this
#define ETH_P_IP    0x0800
#define ETH_P_ARP   0x0806

typedef struct {
    uint8_t  dest[ETH_ALEN];
    uint8_t  src[ETH_ALEN];
    uint16_t type;              // network byte order
} __attribute__((packed)) eth_header_t;

extern const uint8_t ETH_BROADCAST[ETH_ALEN];   // ff:ff:ff:ff:ff:ff

// A frame from netdev's backlog: check it, pull the header, hand the
// payload to ARP or IPv4. Takes the packet.
void ethernet_receive(packet_t *p);

// Put an Ethernet header in front of p and send it on dev. Takes the packet.
int ethernet_send(netdev_t *dev, const uint8_t dest[ETH_ALEN], uint16_t type, packet_t *p);

// Frames nobody upstairs wanted (an unknown type): `ip -s link` shows them.
uint32_t ethernet_unknown_count(void);

#endif
