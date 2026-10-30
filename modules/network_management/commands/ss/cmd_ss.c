/*
 * cmd_ss.c - `ss`: every socket, one line each (iproute2).
 *
 *   ss -tln         listening TCP ports
 *   ss -tap         all TCP sockets, with their process
 *   ss -u -a        UDP sockets
 *   ss -x           UNIX sockets
 *   ss -s           a summary: how many of each, in which state
 *
 * Without -l or -a only connected sockets are listed (as in Linux).
 * An "orphan" is a TCP connection whose program already closed it: the
 * kernel finishes it alone (FIN, TIME_WAIT).
 *
 * Uses: tcp_* / udp_* / unix_* (core), process_get_by_pid
 *       (process_management), inet_format (core/inet), netdev_lock,
 *       arg_next, format, ui_*.
 */

#include "modules/network_management/commands/ss/cmd_ss.h"          // this command
#include "modules/network_management/core/tcp/tcp.h"                // tcp_conn, tcp_state_name
#include "modules/network_management/core/udp/udp.h"                // udp_endpoint
#include "modules/network_management/core/unix/unix.h"              // unix_get
#include "modules/network_management/core/socket/socket.h"          // socket_t
#include "modules/network_management/core/netdev/netdev.h"          // netdev_lock
#include "modules/network_management/core/inet/inet.h"              // inet_format
#include "modules/process_management/core/process/process.h"        // process_get_by_pid
#include "lib/args/args.h"                                          // arg_next
#include "lib/format/format.h"                                      // format
#include "modules/ui/core/style/ui.h"                               // ui_*

static void addr_port(char *out, int size, uint32_t ip, uint16_t port) {
    char addr[INET_ADDR_TEXT];
    inet_format(ip, addr);
    if (port)
        format(out, size, "%s:%u", addr, port);
    else
        format(out, size, "%s:*", addr);
}

static void process_text(char *out, int size, uint32_t pid) {
    process_t *p = pid ? process_get_by_pid(pid) : 0;
    if (p)
        format(out, size, "users:((\"%s\",pid=%u))", p->name, pid);
    else
        out[0] = '\0';
}

static void line(const char *netid, const char *state, uint32_t rq, uint32_t sq,
                 const char *local, const char *peer, const char *proc) {
    ui_printf("%-5s %-10s %-6u %-6u %-21s %-21s %s\n", netid, state, rq, sq, local, peer, proc);
}

static void summary(void) {
    int tcp = 0, estab = 0, closed = 0, orphan = 0, timewait = 0, listen = 0, udp = 0, un = 0;
    for (int i = 0; i < TCP_MAX_CONNS; i++) {
        const tcp_conn_t *c = tcp_conn(i);
        if (!c)
            continue;
        tcp++;
        estab += c->state == TCP_ESTABLISHED;
        closed += c->state == TCP_CLOSED;
        timewait += c->state == TCP_TIME_WAIT;
        listen += c->state == TCP_LISTEN;
        orphan += !c->sock && !c->listener && c->state != TCP_LISTEN;
    }
    for (int i = 0; i < UDP_MAX_ENDPOINTS; i++)
        udp += udp_endpoint(i) != 0;
    for (int i = 0; i < UNIX_MAX; i++)
        un += unix_get(i) != 0;
    ui_printf("Total: %d\n", tcp + udp + un);
    ui_printf("TCP:   %d (estab %d, closed %d, orphaned %d, timewait %d, listening %d)\n",
              tcp, estab, closed, orphan, timewait, listen);
    ui_color(UI_TITLE);
    ui_printf("\nTransport Total\n");
    ui_reset();
    ui_printf("UDP       %d\nTCP       %d\nUNIX      %d\n", udp, tcp, un);
}

void cmd_ss(const char *args) {
    int tcp = 0, udp = 0, unx = 0, listening = 0, all = 0, procs = 0, stats = 0;
    char word[16];
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (word[0] != '-') {
            ui_usage("ss [-tuxlanp] | ss -s");
            return;
        }
        for (const char *c = word + 1; *c; c++) {
            switch (*c) {
            case 't': tcp = 1; break;
            case 'u': udp = 1; break;
            case 'x': unx = 1; break;
            case 'l': listening = 1; break;
            case 'a': all = 1; break;
            case 'p': procs = 1; break;
            case 's': stats = 1; break;
            case 'n': break;                        // UzOS always prints numbers
            default:
                ui_usage("ss [-tuxlanp] | ss -s");
                return;
            }
        }
    }
    uint32_t lock = netdev_lock();
    if (stats) {
        summary();
        netdev_unlock(lock);
        return;
    }
    if (!tcp && !udp && !unx)
        tcp = udp = unx = 1;

    char local[24], peer[24], proc[48];
    ui_color(UI_TITLE);
    ui_printf("%-5s %-10s %-6s %-6s %-21s %-21s %s\n", "Netid", "State", "Recv-Q", "Send-Q",
              "Local Address:Port", "Peer Address:Port", procs ? "Process" : "");
    ui_reset();
    for (int i = 0; tcp && i < TCP_MAX_CONNS; i++) {
        const tcp_conn_t *c = tcp_conn(i);
        if (!c)
            continue;
        int is_listen = c->state == TCP_LISTEN;
        if (!all && (listening ? !is_listen : is_listen))
            continue;
        addr_port(local, sizeof(local), c->local_ip, c->local_port);
        addr_port(peer, sizeof(peer), c->remote_ip, c->remote_port);
        process_text(proc, sizeof(proc), procs && c->sock ? c->sock->pid : 0);
        // A listener's queues are its waiting connections and its backlog.
        line("tcp", tcp_state_name(c->state), is_listen ? (uint32_t)c->pending : c->rcv_len,
             is_listen ? (uint32_t)c->backlog : c->snd_len, local, peer, proc);
    }
    for (int i = 0; udp && i < UDP_MAX_ENDPOINTS; i++) {
        const udp_endpoint_t *ep = udp_endpoint(i);
        if (!ep || (!all && !listening && !ep->remote_ip) || (listening && ep->remote_ip))
            continue;
        const socket_t *s = ep->owner;
        addr_port(local, sizeof(local), ep->local_ip, ep->local_port);
        addr_port(peer, sizeof(peer), ep->remote_ip, ep->remote_port);
        process_text(proc, sizeof(proc), procs && s ? s->pid : 0);
        line("udp", ep->remote_ip ? "ESTAB" : "UNCONN", s ? s->rx.bytes : 0, 0, local, peer, proc);
    }
    for (int i = 0; unx && i < UNIX_MAX; i++) {
        const unix_sock_t *u = unix_get(i);
        if (!u)
            continue;
        int is_listen = u->sock && u->sock->state == SS_LISTENING;
        if (!all && (listening ? !is_listen : is_listen))
            continue;
        process_text(proc, sizeof(proc), procs && u->sock ? u->sock->pid : 0);
        line(u->sock && u->sock->type == SOCK_DGRAM ? "u_dgr" : "u_str", is_listen ? "LISTEN" : "ESTAB",
             u->len, 0, u->path[0] ? u->path : "*", u->peer && u->peer->path[0] ? u->peer->path : "*", proc);
    }
    netdev_unlock(lock);
}
