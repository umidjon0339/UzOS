/*
 * netdev.c - The interface table, the receive backlog, sending.
 *
 * Uses: packet_* (core/packet), ethernet_receive (core/ethernet), arp_flush (core/arp),
 *       route_add / route_flush_dev (core/route),
 *       cpu_irq_save / cpu_irq_restore (arch/x86/cpu), strcmp (lib/string).
 */

#include "modules/network_management/core/netdev/netdev.h"       // this file
#include "modules/network_management/core/ethernet/ethernet.h"   // ethernet_receive
#include "modules/network_management/core/arp/arp.h"             // arp_flush
#include "modules/network_management/core/route/route.h"         // route_add, route_flush_dev
#include "arch/x86/cpu/cpu.h"                                     // cpu_irq_save, cpu_irq_restore
#include "lib/string/string.h"                                    // strcmp, memcpy
#include "modules/network_management/core/inet/inet.h"           // NET_EPERM
#include "modules/network_management/security/promiscuous_sniffing/defence/promisc_cap/def_promisc_cap.h"  // def_promisc_ok
#include "modules/network_management/security/mac_spoofing/defence/mac_lock/def_mac_lock.h"               // def_mac_change_ok

static netdev_t *devices[NETDEV_MAX];
static int device_count;

static packet_queue_t backlog;          // received, not yet passed up
static netdev_tap_t tap;                // tcpdump, while it runs
static int draining;                    // 1 while backlog frames go up the stack

/* ---------- The table ---------- */

int netdev_register(netdev_t *dev) {
    if (device_count >= NETDEV_MAX)
        return 0;
    devices[device_count++] = dev;
    dev->index = device_count;
    return dev->index;
}

int netdev_count(void) {
    return device_count;
}

netdev_t *netdev_get(int position) {
    return position >= 0 && position < device_count ? devices[position] : 0;
}

netdev_t *netdev_by_name(const char *name) {
    for (int i = 0; i < device_count; i++)
        if (strcmp(devices[i]->name, name) == 0)
            return devices[i];
    return 0;
}

netdev_t *netdev_by_index(int index) {
    return netdev_get(index - 1);
}

int netdev_running(netdev_t *dev) {
    return (dev->flags & NETDEV_UP) && (!dev->link_up || dev->link_up(dev));
}

void netdev_set_up(netdev_t *dev, int up) {
    dev->flags = up ? dev->flags | NETDEV_UP : dev->flags & ~NETDEV_UP;
}

void netdev_set_address(netdev_t *dev, uint32_t ip, uint32_t netmask) {
    uint32_t flags = cpu_irq_save();
    dev->ip = ip;
    dev->netmask = ip ? netmask : 0;
    arp_flush(dev);
    // Routes through dev belonged to the old address. The new one
    // makes its own network reachable directly: "10.0.2.0/24 dev eth0".
    route_flush_dev(dev);
    if (ip)
        route_add(ip & netmask, netmask, 0, dev, 0, ROUTE_KERNEL);
    cpu_irq_restore(flags);
}

int netdev_set_promisc(netdev_t *dev, int on, uint32_t uid) {
    if (!def_promisc_ok(uid))               // DEFENCE promisc_cap: sniffing is root's
        return -NET_EPERM;
    dev->flags = on ? dev->flags | NETDEV_PROMISC : dev->flags & ~NETDEV_PROMISC;
    if (dev->set_promisc)
        dev->set_promisc(dev, on);
    return 0;
}

int netdev_set_mac(netdev_t *dev, const uint8_t mac[NETDEV_MAC_LEN], uint32_t uid) {
    if (!def_mac_change_ok(uid))            // DEFENCE mac_lock: the card's identity is root's
        return -NET_EPERM;
    uint32_t flags = cpu_irq_save();
    memcpy(dev->mac, mac, NETDEV_MAC_LEN);
    if (dev->set_mac)
        dev->set_mac(dev, mac);             // the card's receive filter too
    arp_flush(dev);
    cpu_irq_restore(flags);
    return 0;
}

/* ---------- Locking ---------- */

uint32_t netdev_lock(void) {
    return cpu_irq_save();
}

void netdev_unlock(uint32_t flags) {
    cpu_irq_restore(flags);
}

/* ---------- Receiving ---------- */

// Pass every waiting frame up, oldest first. Frames that arrive while
// we are at it (lo replies) join the queue and are handled in this loop.
static void drain(void) {
    draining = 1;
    packet_t *p;
    while ((p = packet_queue_pop(&backlog))) {
        if (!(p->dev->flags & NETDEV_UP)) {
            p->dev->stats.rx_dropped++;     // interface is down: deaf
            packet_free(p);
            continue;
        }
        ethernet_receive(p);                // takes the packet
    }
    draining = 0;
}

void netdev_set_tap(netdev_tap_t fn) {
    tap = fn;
}

void netdev_receive(netdev_t *dev, const uint8_t *frame, uint16_t len) {
    uint32_t flags = cpu_irq_save();
    if (tap)
        tap(dev, frame, len, 0);
    packet_t *p = packet_from(frame, len);
    if (!p) {
        dev->stats.rx_dropped++;            // no buffer free (or too long)
    } else {
        p->dev = dev;
        dev->stats.rx_packets++;
        dev->stats.rx_bytes += len;
        packet_queue_push(&backlog, p);
        if (!draining)
            drain();
    }
    cpu_irq_restore(flags);
}

void netdev_inject(netdev_t *dev, const uint8_t *frame, uint16_t len) {
    netdev_receive(dev, frame, len);        // the very same path as the card's interrupt
}

/* ---------- Sending ---------- */

int netdev_transmit(netdev_t *dev, packet_t *p) {
    uint32_t flags = cpu_irq_save();
    int sent = 0;
    if (!netdev_running(dev)) {
        dev->stats.tx_dropped++;            // down, or no cable
    } else {
        // tcpdump sees what goes out (lo's frames it sees once: coming in).
        if (tap && !(dev->flags & NETDEV_LOOPBACK))
            tap(dev, p->data, p->len, 1);
        if (dev->transmit(dev, p)) {
            dev->stats.tx_packets++;
            dev->stats.tx_bytes += p->len;
            sent = 1;
        } else {
            dev->stats.tx_errors++;         // the driver's ring was full
        }
    }
    packet_free(p);
    cpu_irq_restore(flags);
    return sent;
}
