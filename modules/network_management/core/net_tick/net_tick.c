/*
 * net_tick.c - Run every layer's timers, NET_TICK_MS apart.
 *
 * Runs inside the timer interrupt: interrupts are off, like everywhere
 * else in the stack, so no layer is ever half-way through a change.
 *
 * Uses: timer_uptime_ms (device_management), arp_tick (core/arp),
 *       ipv4_fragment_tick (core/ipv4), dhcp_tick (core/dhcp), tcp_tick (core/tcp).
 */

#include "modules/network_management/core/net_tick/net_tick.h"   // this file
#include "modules/network_management/core/arp/arp.h"             // arp_tick
#include "modules/network_management/core/ipv4/ipv4.h"           // ipv4_fragment_tick
#include "modules/network_management/core/dhcp/dhcp.h"           // dhcp_tick
#include "modules/network_management/core/tcp/tcp.h"             // tcp_tick
#include "modules/device_management/timer/timer.h"               // timer_uptime_ms

static uint32_t last_run;

uint32_t net_now_ms(void) {
    return timer_uptime_ms();
}

void net_tick(void) {
    uint32_t now = net_now_ms();
    if (now - last_run < NET_TICK_MS)
        return;
    last_run = now;
    arp_tick(now);
    ipv4_fragment_tick(now);
    dhcp_tick(now);
    tcp_tick(now);
}
