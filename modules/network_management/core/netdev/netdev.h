#ifndef NETDEV_H
#define NETDEV_H

/*
 * netdev.h - Network interfaces: "lo" and "eth0".
 *
 * An interface is what the rest of the stack sends through and hears
 * from, whatever is underneath: the e1000 card (eth0), or nothing at
 * all (lo, the loopback, hands every frame straight back).
 *
 *            ARP / IPv4 / ...
 *                  |  ethernet_send()        ^ ethernet_receive()
 *                  v                         |
 *   netdev_transmit(dev, packet)      netdev_receive(dev, frame)
 *                  |                         ^
 *          dev->transmit()            driver's interrupt
 *                  v                         |
 *              e1000 card  ------ wire ------+
 *
 * Received frames wait in one "backlog" queue and are passed up one at
 * a time, never inside each other. That matters for lo: sending on lo
 * receives at once, and the reply sends again; without the queue, every
 * reply would go one call deeper on the stack.
 *
 * Everything in the stack runs with interrupts off: the card's
 * interrupt, and any process that enters the stack (netdev_lock()).
 */

#include <stdint.h>
#include "modules/network_management/core/packet/packet.h"     // packet_t

#define NETDEV_MAX      4
#define NETDEV_NAME_LEN 8
#define NETDEV_MAC_LEN  6

// Flags, the same bits as Linux's IFF_* (ip link prints their names).
#define NETDEV_UP        0x1        // turned on (ip link set dev up)
#define NETDEV_BROADCAST 0x2        // can reach everyone on the link
#define NETDEV_LOOPBACK  0x8        // lo
#define NETDEV_PROMISC   0x100      // receives frames for every MAC
#define NETDEV_MULTICAST 0x1000

typedef struct {
    uint32_t rx_packets, rx_bytes, rx_errors, rx_dropped;
    uint32_t tx_packets, tx_bytes, tx_errors, tx_dropped;
} netdev_stats_t;

typedef struct netdev {
    int index;                              // 1 = lo, 2 = eth0 (ifindex)
    char name[NETDEV_NAME_LEN];
    uint8_t mac[NETDEV_MAC_LEN];            // 00:00:00:00:00:00 on lo
    uint32_t flags;                         // NETDEV_*
    uint16_t mtu;                           // biggest IP packet it carries
    uint32_t ip, netmask;                   // host order; 0 = no address yet

    // The driver. transmit() puts one whole frame out and does NOT keep
    // the packet (it copies); returns 1 on success.
    int  (*transmit)(struct netdev *dev, packet_t *p);
    int  (*link_up)(struct netdev *dev);    // carrier: cable in? (NULL = always)
    void (*set_promisc)(struct netdev *dev, int on);
    void (*set_mac)(struct netdev *dev, const uint8_t mac[NETDEV_MAC_LEN]);

    netdev_stats_t stats;
} netdev_t;

// Add an interface (the driver fills name, mac, flags, mtu, transmit).
// Returns its index, or 0 if the table is full.
int netdev_register(netdev_t *dev);

int netdev_count(void);
netdev_t *netdev_get(int position);         // 0 .. count-1, in index order
netdev_t *netdev_by_name(const char *name);
netdev_t *netdev_by_index(int index);

// Is the link working: turned on AND carrier present (LOWER_UP)?
int netdev_running(netdev_t *dev);

void netdev_set_up(netdev_t *dev, int up);

// Give dev an address (ip addr add, DHCP); ip 0 removes it. Forgets
// the neighbours and routes of the old network, and adds the route to
// the new one ("10.0.2.0/24 dev eth0").
void netdev_set_address(netdev_t *dev, uint32_t ip, uint32_t netmask);
// Promiscuous mode on/off, for user 'uid'. 0, or -NET_EPERM: needs
// CAP_NET_RAW (DEFENCE promisc_cap).
int netdev_set_promisc(netdev_t *dev, int on, uint32_t uid);

// A new MAC address, for user 'uid' (ip link set dev address). 0, or
// -NET_EPERM: needs CAP_NET_ADMIN (DEFENCE mac_lock).
int netdev_set_mac(netdev_t *dev, const uint8_t mac[NETDEV_MAC_LEN], uint32_t uid);

// A driver got a frame: copy it into a packet and pass it up the stack.
// Safe in an interrupt.
void netdev_receive(netdev_t *dev, const uint8_t *frame, uint16_t len);

// The security lab's wire: hand 'frame' to the stack exactly as if the
// card had received it on dev. Attacks use it to send what a machine on
// the link could send; the defences then meet it in the real path.
void netdev_inject(netdev_t *dev, const uint8_t *frame, uint16_t len);

// Send a finished frame (Ethernet header already in front). Always
// takes the packet: it is freed here, sent or not. Returns 1 if sent.
int netdev_transmit(netdev_t *dev, packet_t *p);

// A tap sees every frame that comes in or goes out on any interface,
// before anything filters it (tcpdump). Called with interrupts off.
typedef void (*netdev_tap_t)(netdev_t *dev, const uint8_t *frame, uint16_t len, int outgoing);
void netdev_set_tap(netdev_tap_t tap);

// A process entering the network stack: interrupts off while it is inside.
//     uint32_t f = netdev_lock(); ... netdev_unlock(f);
uint32_t netdev_lock(void);
void netdev_unlock(uint32_t flags);

#endif
