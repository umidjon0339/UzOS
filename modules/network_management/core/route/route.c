/*
 * route.c - Keep the routing table and look destinations up in it.
 *
 * Uses: netdev_* (core/netdev), loopback_dev (core/loopback), NET_E*
 *       (core/inet), cpu_irq_save / cpu_irq_restore (arch/x86/cpu).
 */

#include "modules/network_management/core/route/route.h"         // this file
#include "modules/network_management/core/loopback/loopback.h"   // loopback_dev
#include "modules/network_management/core/inet/inet.h"           // NET_E*
#include "arch/x86/cpu/cpu.h"                                     // cpu_irq_save, cpu_irq_restore

static route_t table[ROUTE_MAX];

int route_add(uint32_t dest, uint32_t mask, uint32_t gateway, netdev_t *dev,
              uint32_t metric, route_proto_t proto) {
    if (!dev || (dest & ~mask))
        return -NET_EINVAL;                 // host bits set in a network address
    uint32_t flags = cpu_irq_save();
    route_t *slot = 0;
    int result = 0;
    for (int i = 0; i < ROUTE_MAX; i++) {
        if (table[i].used && table[i].dest == dest && table[i].mask == mask &&
            table[i].metric == metric) {
            result = -NET_EADDRINUSE;       // Linux: "File exists"
            break;
        }
        if (!table[i].used && !slot)
            slot = &table[i];
    }
    if (!result && !slot)
        result = -NET_ENOBUFS;
    if (!result) {
        slot->used = 1;
        slot->dest = dest;
        slot->mask = mask;
        slot->gateway = gateway;
        slot->dev = dev;
        slot->metric = metric;
        slot->proto = proto;
    }
    cpu_irq_restore(flags);
    return result;
}

int route_del(uint32_t dest, uint32_t mask) {
    uint32_t flags = cpu_irq_save();
    int found = 0;
    for (int i = 0; i < ROUTE_MAX && !found; i++) {
        if (table[i].used && table[i].dest == dest && table[i].mask == mask) {
            table[i].used = 0;
            found = 1;
        }
    }
    cpu_irq_restore(flags);
    return found ? 0 : -NET_EINVAL;
}

void route_flush_dev(netdev_t *dev) {
    uint32_t flags = cpu_irq_save();
    for (int i = 0; i < ROUTE_MAX; i++)
        if (table[i].used && table[i].dev == dev)
            table[i].used = 0;
    cpu_irq_restore(flags);
}

int route_is_local(uint32_t ip) {
    for (int i = 0; i < netdev_count(); i++)
        if (netdev_get(i)->ip == ip)
            return 1;
    return (ip >> 24) == 127;               // all of 127.x.x.x is this machine
}

int route_lookup(uint32_t dest, route_result_t *out) {
    // Our own address: never on the wire.
    if (route_is_local(dest)) {
        out->dev = loopback_dev();
        out->next_hop = dest;
        out->src = dest;
        return 1;
    }

    const route_t *best = 0;
    for (int i = 0; i < ROUTE_MAX; i++) {
        const route_t *r = &table[i];
        if (!r->used || (dest & r->mask) != r->dest || !netdev_running(r->dev))
            continue;
        if (!best || r->mask > best->mask ||          // longer prefix = bigger mask
            (r->mask == best->mask && r->metric < best->metric))
            best = r;
    }
    if (!best)
        return 0;
    out->dev = best->dev;
    out->next_hop = best->gateway ? best->gateway : dest;
    out->src = best->dev->ip;
    return 1;
}

const route_t *route_get(int slot) {
    return slot >= 0 && slot < ROUTE_MAX ? &table[slot] : 0;
}
