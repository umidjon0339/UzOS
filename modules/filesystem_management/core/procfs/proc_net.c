/*
 * proc_net.c - Write the files of /proc/net from network_management's tables.
 *
 * The layouts are Linux's, so a reader who knows Linux finds the same
 * columns. Linux prints an IPv4 address in /proc/net/tcp as the 32-bit
 * number stored in network byte order, read on a little-endian CPU:
 * 127.0.0.1 appears as 0100007F. Ports are plain hex (80 = 0050).
 *
 * Uses: netdev_* / arp_* / route_* / tcp_* / udp_* / unix_* / ipv4_stats /
 *       icmp_stats / socket_get (network_management/core), htonl, proc_printf.
 */

#include "modules/filesystem_management/core/procfs/proc_net.h"   // this file
#include "modules/network_management/core/netdev/netdev.h"       // netdev_count, netdev_get, netdev_lock
#include "modules/network_management/core/arp/arp.h"             // arp_entry
#include "modules/network_management/core/route/route.h"         // route_get
#include "modules/network_management/core/tcp/tcp.h"             // tcp_conn, tcp_stats
#include "modules/network_management/core/udp/udp.h"             // udp_endpoint, udp_stats
#include "modules/network_management/core/unix/unix.h"           // unix_get
#include "modules/network_management/core/ipv4/ipv4.h"           // ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"           // icmp_stats
#include "modules/network_management/core/inet/inet.h"           // htonl

const char *proc_net_name(proc_net_file_t which) {
    static const char *names[PROC_NET_FILES] = { "dev", "arp", "route", "tcp", "udp", "unix", "snmp" };
    return names[which];
}

// Linux's state numbers in /proc/net/tcp.
static int linux_state(tcp_state_t s) {
    static const int numbers[] = { 7, 0x0A, 2, 3, 1, 4, 5, 8, 0x0B, 9, 6 };
    return numbers[s];
}

static void dev(proc_buf_t *buf) {
    proc_printf(buf, "Inter-|   Receive                      |  Transmit\n");
    proc_printf(buf, " face |bytes    packets errs drop     |bytes    packets errs drop\n");
    for (int i = 0; i < netdev_count(); i++) {
        netdev_t *d = netdev_get(i);
        const netdev_stats_t *s = &d->stats;
        proc_printf(buf, "%6s: %8u %7u %4u %4u      %8u %7u %4u %4u\n", d->name,
                    s->rx_bytes, s->rx_packets, s->rx_errors, s->rx_dropped,
                    s->tx_bytes, s->tx_packets, s->tx_errors, s->tx_dropped);
    }
}

static void arp(proc_buf_t *buf) {
    char ip[INET_ADDR_TEXT], mac[MAC_TEXT];
    proc_printf(buf, "IP address       HW type     Flags       HW address            Mask     Device\n");
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        const arp_entry_t *e = arp_entry(i);
        if (e->state == ARP_FREE)
            continue;
        inet_format(e->ip, ip);
        mac_format(e->mac, mac);
        // Flags: 0x2 = complete, 0x4 = permanent, 0x0 = still asking.
        int flags = e->state == ARP_INCOMPLETE ? 0 : e->state == ARP_PERMANENT ? 0x6 : 0x2;
        proc_printf(buf, "%-16s 0x1         0x%x         %s     *        %s\n", ip, flags,
                    mac, e->dev->name);
    }
}

static void route(proc_buf_t *buf) {
    proc_printf(buf, "Iface\tDestination\tGateway \tFlags\tMetric\tMask\n");
    for (int i = 0; i < ROUTE_MAX; i++) {
        const route_t *r = route_get(i);
        if (!r->used)
            continue;
        int flags = 0x1 | (r->gateway ? 0x2 : 0);           // RTF_UP, RTF_GATEWAY
        proc_printf(buf, "%s\t%08x\t%08x\t%04x\t%u\t%08x\n", r->dev->name, htonl(r->dest),
                    htonl(r->gateway), flags, r->metric, htonl(r->mask));
    }
}

static void tcp(proc_buf_t *buf) {
    proc_printf(buf, "  sl  local_address rem_address   st tx_queue rx_queue  uid\n");
    int n = 0;
    for (int i = 0; i < TCP_MAX_CONNS; i++) {
        const tcp_conn_t *c = tcp_conn(i);
        if (!c)
            continue;
        proc_printf(buf, "%4d: %08x:%04x %08x:%04x %02x %08x:%08x %4u\n", n++,
                    htonl(c->local_ip), c->local_port, htonl(c->remote_ip), c->remote_port,
                    linux_state(c->state), c->snd_len, c->rcv_len, c->uid);
    }
}

static void udp(proc_buf_t *buf) {
    proc_printf(buf, "  sl  local_address rem_address   st  uid\n");
    int n = 0;
    for (int i = 0; i < UDP_MAX_ENDPOINTS; i++) {
        const udp_endpoint_t *ep = udp_endpoint(i);
        if (!ep)
            continue;
        proc_printf(buf, "%4d: %08x:%04x %08x:%04x 07 %4u\n", n++, htonl(ep->local_ip),
                    ep->local_port, htonl(ep->remote_ip), ep->remote_port, ep->uid);
    }
}

static void unix_sockets(proc_buf_t *buf) {
    proc_printf(buf, "Num  Type St Path\n");
    for (int i = 0; i < UNIX_MAX; i++) {
        const unix_sock_t *u = unix_get(i);
        if (!u)
            continue;
        int type = u->sock ? u->sock->type : SOCK_STREAM;
        int st = !u->sock ? 2 : u->sock->state == SS_LISTENING ? 1 : u->peer ? 3 : 0;
        proc_printf(buf, "%3d: %04x %02x %s\n", i, type, st, u->path);
    }
}

static void snmp(proc_buf_t *buf) {
    const ipv4_stats_t *ip = ipv4_stats();
    const icmp_stats_t *ic = icmp_stats();
    const tcp_stats_t *t = tcp_stats();
    const udp_stats_t *u = udp_stats();
    proc_printf(buf, "Ip: InReceives InHdrErrors InAddrErrors InUnknownProtos InDelivers OutRequests "
                     "OutNoRoutes ReasmReqds ReasmOKs ReasmFails FragOKs FragFails FragCreates\n");
    proc_printf(buf, "Ip: %u %u %u %u %u %u %u %u %u %u %u %u %u\n", ip->in_receives, ip->in_hdr_errors,
                ip->in_addr_errors, ip->in_unknown_protos, ip->in_delivers, ip->out_requests,
                ip->out_no_routes, ip->reasm_reqds, ip->reasm_oks, ip->reasm_fails, ip->frag_oks,
                ip->frag_fails, ip->frag_creates);
    proc_printf(buf, "Icmp: InMsgs InErrors InEchos InEchoReps InDestUnreachs InTimeExcds "
                     "OutMsgs OutEchos OutEchoReps OutDestUnreachs\n");
    proc_printf(buf, "Icmp: %u %u %u %u %u %u %u %u %u %u\n", ic->in_msgs, ic->in_errors, ic->in_echos,
                ic->in_echo_reps, ic->in_dest_unreachs, ic->in_time_excds, ic->out_msgs, ic->out_echos,
                ic->out_echo_reps, ic->out_dest_unreachs);
    proc_printf(buf, "Tcp: ActiveOpens PassiveOpens AttemptFails EstabResets InSegs OutSegs "
                     "RetransSegs InErrs OutRsts BacklogDrops\n");
    proc_printf(buf, "Tcp: %u %u %u %u %u %u %u %u %u %u\n", t->active_opens, t->passive_opens,
                t->attempt_fails, t->estab_resets, t->in_segs, t->out_segs, t->retrans_segs,
                t->in_errs, t->out_rsts, t->backlog_drops);
    proc_printf(buf, "Udp: InDatagrams NoPorts InErrors OutDatagrams RcvbufErrors\n");
    proc_printf(buf, "Udp: %u %u %u %u %u\n", u->in_datagrams, u->no_ports, u->in_errors,
                u->out_datagrams, u->rcvbuf_errors);
}

void proc_net(proc_buf_t *buf, proc_net_file_t which) {
    uint32_t lock = netdev_lock();          // a consistent picture: the network may not change meanwhile
    switch (which) {
    case PROC_NET_DEV:   dev(buf);          break;
    case PROC_NET_ARP:   arp(buf);          break;
    case PROC_NET_ROUTE: route(buf);        break;
    case PROC_NET_TCP:   tcp(buf);          break;
    case PROC_NET_UDP:   udp(buf);          break;
    case PROC_NET_UNIX:  unix_sockets(buf); break;
    case PROC_NET_SNMP:  snmp(buf);         break;
    default:                                break;
    }
    netdev_unlock(lock);
}
