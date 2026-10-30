/*
 * cmd_netstat.c - `netstat`: who talks to whom (net-tools).
 *
 *   netstat -tuln      listening TCP and UDP ports, numbers only
 *   netstat -tanp      every TCP connection, with the program behind it
 *   netstat -x         AF_UNIX sockets
 *   netstat -r         the routing table          (like route -n)
 *   netstat -i         the interfaces' counters
 *   netstat -s         counters per protocol      (from /proc/net/snmp)
 *
 *   -t TCP  -u UDP  -x UNIX  -l listening only  -a all  -n numbers  -p program
 *
 * With no -l or -a, listening sockets are left out (as in Linux).
 *
 * Uses: tcp_* / udp_* / unix_* / netdev_* / route_* / ipv4_stats /
 *       icmp_stats (core), process_get_by_pid (process_management),
 *       inet_format (core/inet), arg_next, format, ui_*.
 */

#include "modules/network_management/commands/netstat/cmd_netstat.h"  // this command
#include "modules/network_management/core/tcp/tcp.h"                // tcp_conn, tcp_state_name, tcp_stats
#include "modules/network_management/core/udp/udp.h"                // udp_endpoint, udp_stats
#include "modules/network_management/core/unix/unix.h"              // unix_get
#include "modules/network_management/core/socket/socket.h"          // socket_t
#include "modules/network_management/core/netdev/netdev.h"          // netdev_*
#include "modules/network_management/core/route/route.h"            // route_get
#include "modules/network_management/core/ipv4/ipv4.h"              // ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"              // icmp_stats
#include "modules/network_management/core/inet/inet.h"              // inet_format
#include "modules/process_management/core/process/process.h"        // process_get_by_pid
#include "lib/args/args.h"                                          // arg_next
#include "lib/format/format.h"                                      // format
#include "modules/ui/core/style/ui.h"                               // ui_*

typedef struct {
    int tcp, udp, unix_, listening, all, numeric, program;
} opts_t;

// "10.0.2.15:80", "0.0.0.0:*" for an unknown peer port.
static void endpoint(char *out, int size, uint32_t ip, uint16_t port) {
    char addr[INET_ADDR_TEXT];
    inet_format(ip, addr);
    if (port)
        format(out, size, "%s:%u", addr, port);
    else
        format(out, size, "%s:*", addr);
}

// "4/httpd" for the socket's process.
static void program(char *out, int size, uint32_t pid) {
    process_t *p = pid ? process_get_by_pid(pid) : 0;
    if (p)
        format(out, size, "%u/%s", pid, p->name);
    else
        format(out, size, "-");
}

static void connections(const opts_t *o) {
    char local[24], remote[24], prog[24];
    ui_printf("Active Internet connections (%s)\n",
              o->all ? "servers and established" : o->listening ? "only servers" : "w/o servers");
    ui_color(UI_TITLE);
    ui_printf("Proto Recv-Q Send-Q %-18s %-18s %-11s%s\n", "Local Address", "Foreign Address",
              "State", o->program ? " PID/Program" : "");
    ui_reset();
    uint32_t lock = netdev_lock();
    for (int i = 0; o->tcp && i < TCP_MAX_CONNS; i++) {
        const tcp_conn_t *c = tcp_conn(i);
        if (!c)
            continue;
        int listen = c->state == TCP_LISTEN;
        if ((listen && !o->all && !o->listening) || (!listen && o->listening && !o->all))
            continue;
        endpoint(local, sizeof(local), c->local_ip, c->local_port);
        endpoint(remote, sizeof(remote), c->remote_ip, c->remote_port);
        program(prog, sizeof(prog), c->sock ? c->sock->pid : 0);
        ui_printf("tcp   %6u %6u %-18s %-18s %-11s%s%s\n", c->rcv_len, c->snd_len, local, remote,
                  tcp_state_name(c->state), o->program ? " " : "", o->program ? prog : "");
    }
    for (int i = 0; o->udp && (o->all || o->listening) && i < UDP_MAX_ENDPOINTS; i++) {
        const udp_endpoint_t *ep = udp_endpoint(i);
        if (!ep)
            continue;
        const socket_t *s = ep->owner;
        endpoint(local, sizeof(local), ep->local_ip, ep->local_port);
        endpoint(remote, sizeof(remote), ep->remote_ip, ep->remote_port);
        program(prog, sizeof(prog), s ? s->pid : 0);
        ui_printf("udp   %6u %6u %-18s %-18s %-11s%s%s\n", s ? s->rx.bytes : 0, 0, local, remote,
                  ep->remote_ip ? "ESTABLISHED" : "", o->program ? " " : "",
                  o->program ? (s ? prog : "kernel") : "");
    }
    netdev_unlock(lock);
}

static void unix_sockets(void) {
    ui_printf("Active UNIX domain sockets\n");
    ui_color(UI_TITLE);
    ui_printf("Proto Type    State        Path\n");
    ui_reset();
    for (int i = 0; i < UNIX_MAX; i++) {
        const unix_sock_t *u = unix_get(i);
        if (!u)
            continue;
        const socket_t *s = u->sock;
        const char *state = !s ? "CONNECTING" : s->state == SS_LISTENING ? "LISTENING"
                          : u->peer ? "CONNECTED" : "";
        ui_printf("unix  %-7s %-12s %s\n", s && s->type == SOCK_DGRAM ? "DGRAM" : "STREAM", state, u->path);
    }
}

static void routes(void) {
    char dest[INET_ADDR_TEXT], gw[INET_ADDR_TEXT], mask[INET_ADDR_TEXT];
    ui_printf("Kernel IP routing table\n");
    ui_color(UI_TITLE);
    ui_printf("%-15s %-15s %-15s Flags Iface\n", "Destination", "Gateway", "Genmask");
    ui_reset();
    for (int i = 0; i < ROUTE_MAX; i++) {
        const route_t *r = route_get(i);
        if (!r->used)
            continue;
        inet_format(r->dest, dest);
        inet_format(r->gateway, gw);
        inet_format(r->mask, mask);
        ui_printf("%-15s %-15s %-15s %-5s %s\n", dest, gw, mask, r->gateway ? "UG" : "U", r->dev->name);
    }
}

static void interfaces(void) {
    ui_printf("Kernel Interface table\n");
    ui_color(UI_TITLE);
    ui_printf("Iface   MTU    RX-OK RX-ERR RX-DRP    TX-OK TX-ERR TX-DRP Flg\n");
    ui_reset();
    for (int i = 0; i < netdev_count(); i++) {
        netdev_t *d = netdev_get(i);
        const netdev_stats_t *s = &d->stats;
        ui_printf("%-6s %5u %8u %6u %6u %8u %6u %6u %s%s%s\n", d->name, d->mtu, s->rx_packets,
                  s->rx_errors, s->rx_dropped, s->tx_packets, s->tx_errors, s->tx_dropped,
                  (d->flags & NETDEV_BROADCAST) ? "B" : "", (d->flags & NETDEV_LOOPBACK) ? "L" : "",
                  netdev_running(d) ? "RU" : "");
    }
}

static void statistics(void) {
    const ipv4_stats_t *ip = ipv4_stats();
    const icmp_stats_t *ic = icmp_stats();
    const tcp_stats_t *t = tcp_stats();
    const udp_stats_t *u = udp_stats();
    ui_color(UI_TITLE); ui_printf("Ip:\n"); ui_reset();
    ui_printf("    %u total packets received, %u with invalid headers, %u with invalid addresses\n",
              ip->in_receives, ip->in_hdr_errors, ip->in_addr_errors);
    ui_printf("    %u incoming packets delivered, %u requests sent out, %u dropped: no route\n",
              ip->in_delivers, ip->out_requests, ip->out_no_routes);
    ui_printf("    %u fragments received, %u reassembled, %u failed; %u fragments created\n",
              ip->reasm_reqds, ip->reasm_oks, ip->reasm_fails, ip->frag_creates);
    ui_color(UI_TITLE); ui_printf("Icmp:\n"); ui_reset();
    ui_printf("    %u messages received (%u echo requests, %u echo replies, %u unreachable)\n",
              ic->in_msgs, ic->in_echos, ic->in_echo_reps, ic->in_dest_unreachs);
    ui_printf("    %u messages sent (%u echo requests, %u echo replies)\n",
              ic->out_msgs, ic->out_echos, ic->out_echo_reps);
    ui_color(UI_TITLE); ui_printf("Tcp:\n"); ui_reset();
    ui_printf("    %u active opens, %u passive opens, %u failed attempts, %u resets received\n",
              t->active_opens, t->passive_opens, t->attempt_fails, t->estab_resets);
    ui_printf("    %u segments received, %u sent, %u retransmitted, %u bad, %u resets sent\n",
              t->in_segs, t->out_segs, t->retrans_segs, t->in_errs, t->out_rsts);
    ui_printf("    %u SYNs dropped: listen queue full\n", t->backlog_drops);
    ui_color(UI_TITLE); ui_printf("Udp:\n"); ui_reset();
    ui_printf("    %u packets received, %u to unknown ports, %u errors, %u sent, %u buffer errors\n",
              u->in_datagrams, u->no_ports, u->in_errors, u->out_datagrams, u->rcvbuf_errors);
}

void cmd_netstat(const char *args) {
    opts_t o = { 0 };
    char word[16];
    char mode = 0;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (word[0] != '-') {
            ui_usage("netstat [-tuxlanp] | -r | -i | -s");
            return;
        }
        for (const char *c = word + 1; *c; c++) {
            switch (*c) {
            case 't': o.tcp = 1; break;
            case 'u': o.udp = 1; break;
            case 'x': o.unix_ = 1; break;
            case 'l': o.listening = 1; break;
            case 'a': o.all = 1; break;
            case 'n': o.numeric = 1; break;             // UzOS always prints numbers
            case 'p': o.program = 1; break;
            case 'r': case 'i': case 's': mode = *c; break;
            default:
                ui_usage("netstat [-tuxlanp] | -r | -i | -s");
                return;
            }
        }
    }
    if (mode == 'r')
        routes();
    else if (mode == 'i')
        interfaces();
    else if (mode == 's')
        statistics();
    else {
        if (!o.tcp && !o.udp && !o.unix_)
            o.tcp = o.udp = o.unix_ = 1;            // none chosen: all three
        if (o.tcp || o.udp)
            connections(&o);
        if (o.unix_)
            unix_sockets();
    }
}
