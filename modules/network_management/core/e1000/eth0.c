/*
 * eth0.c - Join the e1000 driver to the stack as interface "eth0".
 *
 * The driver only knows frames and registers; netdev only knows
 * interfaces. These few functions translate between the two.
 *
 * Uses: e1000_* (this folder), netdev_register / netdev_receive
 *       (core/netdev), strcpy (lib/string).
 */

#include "modules/network_management/core/e1000/e1000.h"         // e1000_*, e1000_register_eth0
#include "modules/network_management/core/netdev/netdev.h"       // netdev_t, netdev_register
#include "lib/string/string.h"                                    // strcpy

static netdev_t eth0;

static int eth0_transmit(netdev_t *dev, packet_t *p) {
    (void)dev;
    return e1000_send(p->data, p->len);
}

static int eth0_link_up(netdev_t *dev) {
    (void)dev;
    return e1000_link_up();
}

static void eth0_set_promisc(netdev_t *dev, int on) {
    (void)dev;
    e1000_set_promiscuous(on);
}

static void eth0_set_mac(netdev_t *dev, const uint8_t mac[NETDEV_MAC_LEN]) {
    (void)dev;
    e1000_set_mac(mac);
}

// The card's interrupt hands us a frame: up into the stack it goes.
static void eth0_receive(const uint8_t *frame, uint16_t len) {
    netdev_receive(&eth0, frame, len);
}

int e1000_register_eth0(void) {
    if (!e1000_present())
        return 0;
    strcpy(eth0.name, "eth0");
    e1000_mac(eth0.mac);
    eth0.flags = NETDEV_UP | NETDEV_BROADCAST | NETDEV_MULTICAST;
    eth0.mtu = 1500;
    eth0.transmit = eth0_transmit;
    eth0.link_up = eth0_link_up;
    eth0.set_promisc = eth0_set_promisc;
    eth0.set_mac = eth0_set_mac;
    e1000_set_receiver(eth0_receive);
    return netdev_register(&eth0);
}
