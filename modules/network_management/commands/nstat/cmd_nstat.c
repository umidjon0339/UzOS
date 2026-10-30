/*
 * cmd_nstat.c - `nstat`: what happened since I last looked.
 *
 *   nstat          #kernel
 *                  IpInReceives      42
 *                  TcpOutSegs        17
 *   nstat -a       the whole count since boot, not the difference
 *   nstat -z       also the counters that did not move
 *
 * The same counters as /proc/net/snmp and `netstat -s`, under Linux's
 * names. The command keeps the last reading, so each run shows only
 * what is new: run it, do something (ping, curl), run it again.
 *
 * Uses: ipv4_stats / icmp_stats / tcp_stats / udp_stats (core), arg_next,
 *       strcmp, ui_*.
 */

#include "modules/network_management/commands/nstat/cmd_nstat.h"    // this command
#include "modules/network_management/core/ipv4/ipv4.h"              // ipv4_stats
#include "modules/network_management/core/icmp/icmp.h"              // icmp_stats
#include "modules/network_management/core/tcp/tcp.h"                // tcp_stats
#include "modules/network_management/core/udp/udp.h"                // udp_stats
#include "lib/args/args.h"                                          // arg_next
#include "lib/string/string.h"                                      // strcmp
#include "modules/ui/core/style/ui.h"                               // ui_*

#define COUNTERS 24

static uint32_t last[COUNTERS];             // the previous run's readings

void cmd_nstat(const char *args) {
    int absolute = 0, zeros = 0;
    char word[8];
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-a") == 0)      absolute = 1;
        else if (strcmp(word, "-z") == 0) zeros = 1;
        else {
            ui_usage("nstat [-a] [-z]");
            return;
        }
    }
    const ipv4_stats_t *ip = ipv4_stats();
    const icmp_stats_t *ic = icmp_stats();
    const tcp_stats_t *t = tcp_stats();
    const udp_stats_t *u = udp_stats();
    const struct { const char *name; uint32_t value; } now[COUNTERS] = {
        { "IpInReceives", ip->in_receives },     { "IpInHdrErrors", ip->in_hdr_errors },
        { "IpInAddrErrors", ip->in_addr_errors },{ "IpInDelivers", ip->in_delivers },
        { "IpOutRequests", ip->out_requests },   { "IpOutNoRoutes", ip->out_no_routes },
        { "IpReasmOKs", ip->reasm_oks },         { "IpFragCreates", ip->frag_creates },
        { "IcmpInMsgs", ic->in_msgs },           { "IcmpInEchos", ic->in_echos },
        { "IcmpInEchoReps", ic->in_echo_reps },  { "IcmpInDestUnreachs", ic->in_dest_unreachs },
        { "IcmpOutMsgs", ic->out_msgs },         { "TcpActiveOpens", t->active_opens },
        { "TcpPassiveOpens", t->passive_opens }, { "TcpAttemptFails", t->attempt_fails },
        { "TcpInSegs", t->in_segs },             { "TcpOutSegs", t->out_segs },
        { "TcpRetransSegs", t->retrans_segs },   { "TcpOutRsts", t->out_rsts },
        { "UdpInDatagrams", u->in_datagrams },   { "UdpNoPorts", u->no_ports },
        { "UdpOutDatagrams", u->out_datagrams }, { "UdpRcvbufErrors", u->rcvbuf_errors },
    };
    ui_color(UI_DIM);
    ui_printf("#kernel\n");
    ui_reset();
    for (int i = 0; i < COUNTERS; i++) {
        uint32_t shown = absolute ? now[i].value : now[i].value - last[i];
        if (shown || zeros)
            ui_printf("%-22s %10u\n", now[i].name, shown);
        last[i] = now[i].value;
    }
}
