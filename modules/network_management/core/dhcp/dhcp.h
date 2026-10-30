#ifndef DHCP_H
#define DHCP_H

/*
 * dhcp.h - DHCP client: ask the network for an address.
 *
 * A new machine has no address, so it cannot be sent anything. It
 * shouts (broadcast, from 0.0.0.0 port 68 to 255.255.255.255 port 67):
 *
 *   us      DISCOVER   "I am 52:54:00:12:34:56, any address for me?"
 *   server  OFFER      "take 10.0.2.15/24, router 10.0.2.2, DNS 10.0.2.3"
 *   us      REQUEST    "I take 10.0.2.15 from server 10.0.2.2"
 *   server  ACK        "it is yours for 24 hours"
 *
 * Then the address, the default route and the DNS server are set
 * (BOUND). Every message carries the "xid" we chose, so answers to
 * other machines' questions are ignored.
 *
 * Nothing proves an OFFER comes from the real server: that is the rogue
 * DHCP attack (security/dhcp_spoofing).
 */

#include <stdint.h>
#include "modules/network_management/core/netdev/netdev.h"     // netdev_t

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_RETRY_MS    2000
#define DHCP_RETRIES     4
#define DHCP_LOG_SIZE    16

// Message types (option 53)
#define DHCPDISCOVER 1
#define DHCPOFFER    2
#define DHCPREQUEST  3
#define DHCPACK      5
#define DHCPNAK      6
#define DHCPRELEASE  7

typedef enum {
    DHCP_OFF,               // not started, or released
    DHCP_SELECTING,         // DISCOVER sent, waiting for an OFFER
    DHCP_REQUESTING,        // REQUEST sent, waiting for the ACK
    DHCP_BOUND,             // we have an address
    DHCP_FAILED,            // no server answered (or it said NAK)
} dhcp_state_t;

typedef struct {
    dhcp_state_t state;
    netdev_t *dev;
    uint32_t xid;           // our transaction id
    uint32_t ip;            // offered / bound address (host order)
    uint32_t server;        // the DHCP server that offered it
    uint32_t netmask, router, dns;
    uint32_t lease_s;       // how long it is ours, in seconds
    uint32_t bound_ms;      // when the ACK came
    int tries;              // messages sent in the current state
    uint32_t sent_ms;       // when the last one went out
} dhcp_lease_t;

// Start (or restart) asking on dev. The rest happens as answers arrive.
void dhcp_start(netdev_t *dev);

// Give the address back to the server and remove it.
void dhcp_release(void);

// Every message sent or received, in order (dhclient prints them).
typedef struct {
    uint8_t type;           // DHCPDISCOVER ...
    uint32_t ip;            // the address it is about
    uint32_t server;
} dhcp_event_t;

int dhcp_log_count(void);                   // events so far, ever
const dhcp_event_t *dhcp_log_get(int n);    // event n; NULL if it is too old

const dhcp_lease_t *dhcp_lease(void);
const char *dhcp_state_name(dhcp_state_t state);

// Resend if no answer came (net_tick).
void dhcp_tick(uint32_t now_ms);

#endif
