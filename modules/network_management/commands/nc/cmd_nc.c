/*
 * cmd_nc.c - `nc`: netcat, the "cat" of the network.
 *
 *   nc example.com 80         connect over TCP; what you type is sent
 *                             line by line, what comes back is printed
 *   nc -u 10.0.2.3 53         the same over UDP (each line one datagram)
 *   nc -l 1234                listen: wait for one connection, then chat
 *   nc -l -u 1234             wait for UDP datagrams on port 1234
 *   nc -z 10.0.2.2 20-25      which TCP ports are open? (connect and close)
 *   -v                        tell more ("Connection to ... succeeded")
 *
 * Ctrl+C ends it. The chat loop never sleeps inside a socket call:
 * the socket is non-blocking, so one loop can watch the keyboard AND
 * the network, a hundred times a second.
 *
 * Uses: socket_* (core/socket), dns_resolve (core/dns), inet_* (core/inet),
 *       keyboard_pop_key (device_management), sys_sleep_interruptible /
 *       signal_interrupted (process_management), arg_next, atoi, ui_*, vga.
 */

#include "modules/network_management/commands/nc/cmd_nc.h"          // this command
#include "modules/network_management/core/socket/socket.h"          // socket_*
#include "modules/network_management/core/dns/dns.h"                // dns_resolve
#include "modules/network_management/core/inet/inet.h"              // inet_format, NET_E*
#include "modules/device_management/keyboard/keyboard.h"            // keyboard_pop_key
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"          // signal_interrupted
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp, strcpy
#include "modules/ui/core/style/ui.h"                               // ui_*

#define USAGE   "nc [-v] [-u] host port | nc -l [-u] port | nc -z host port[-port]"
#define STEP_MS 10
#define LINE_MAX 200
#define CONNECT_WAIT_MS 3000            // -z: how long one port may take

typedef struct {
    int listen, udp, scan, verbose;
    char host[DNS_NAME_MAX];
    int port, port_last;
} options_t;

static void print_error(const char *what, int err) {
    static const struct { int err; const char *text; } texts[] = {
        { NET_ECONNREFUSED, "Connection refused" }, { NET_ETIMEDOUT, "Connection timed out" },
        { NET_ENETUNREACH, "Network is unreachable" }, { NET_EHOSTUNREACH, "No route to host" },
        { NET_EADDRINUSE, "Address already in use" }, { NET_EACCES, "Permission denied" },
        { NET_ECONNRESET, "Connection reset by peer" }, { NET_EINTR, "Interrupted" },
    };
    const char *text = "error";
    for (unsigned i = 0; i < sizeof(texts) / sizeof(texts[0]); i++)
        if (texts[i].err == -err)
            text = texts[i].text;
    ui_start(UI_ERROR, "nc");
    ui_printf("%s: %s", what, text);
    ui_end();
}

/* ---------- The chat loop ---------- */

// Keys -> a line; Enter sends it (to the connected peer). Network data
// -> the screen. Until the peer closes, or Ctrl+C.
static void chat(socket_t *s) {
    static char line[LINE_MAX];
    static uint8_t buf[512];
    int len = 0;
    while (!signal_interrupted()) {
        // The network first: print whatever came.
        int n;
        while ((n = socket_recvfrom(s, buf, sizeof(buf) - 1, MSG_DONTWAIT, 0, 0)) > 0) {
            buf[n] = '\0';
            ui_color(UI_ACCENT);
            ui_printf("%s", (char *)buf);
            ui_reset();
        }
        if (n == 0 && s->type == SOCK_STREAM) {
            ui_hint("(the other side closed the connection)");
            return;
        }
        if (n < 0 && n != -NET_EAGAIN) {
            print_error("recv", n);
            return;
        }
        // Then the keyboard.
        char key;
        while ((key = keyboard_pop_key()) != 0) {
            if (key == '\n') {
                ui_printf("\n");
                line[len++] = '\n';
                int r = socket_sendto(s, line, (uint32_t)len, 0, 0, 0);
                if (r < 0) {
                    print_error("send", r);
                    return;
                }
                len = 0;
            } else if (key == '\b' && len > 0) {
                len--;
                ui_printf("\b \b");
            } else if (key >= ' ' && key < 127 && len < LINE_MAX - 2) {
                line[len++] = key;
                ui_printf("%c", key);
            }
        }
        sys_sleep_interruptible(STEP_MS);
    }
}

/* ---------- Modes ---------- */

static void client(const options_t *o, uint32_t ip) {
    socket_t *s;
    int r = socket_create(AF_INET, o->udp ? SOCK_DGRAM : SOCK_STREAM, 0, &s);
    if (r < 0) {
        print_error("socket", r);
        return;
    }
    sockaddr_t to;
    uint32_t to_len = sizeof(to);
    socket_inet_addr(&to, &to_len, ip, (uint16_t)o->port);
    r = socket_connect(s, &to, to_len);         // blocks until TCP says yes or no
    if (r < 0) {
        print_error("connect", r);
        socket_close(s);
        return;
    }
    if (o->verbose)
        ui_printf("Connection to %s %d port [%s] succeeded!\n", o->host, o->port, o->udp ? "udp" : "tcp");
    s->nonblock = 1;
    chat(s);
    socket_close(s);
}

static void server(const options_t *o) {
    socket_t *l;
    int r = socket_create(AF_INET, o->udp ? SOCK_DGRAM : SOCK_STREAM, 0, &l);
    if (r < 0) {
        print_error("socket", r);
        return;
    }
    int one = 1;
    socket_setopt(l, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    sockaddr_t me;
    uint32_t me_len = sizeof(me);
    socket_inet_addr(&me, &me_len, 0, (uint16_t)o->port);
    if ((r = socket_bind(l, &me, me_len)) < 0 || (!o->udp && (r = socket_listen(l, 1)) < 0)) {
        print_error("bind", r);
        socket_close(l);
        return;
    }
    ui_printf("Listening on 0.0.0.0 %d (%s) - Ctrl+C to stop\n", o->port, o->udp ? "udp" : "tcp");

    if (o->udp) {
        // UDP has no connection: wait for the first datagram, then
        // "connect" to its sender, so what we type goes back there.
        static uint8_t first[512];
        sockaddr_t from;
        uint32_t from_len = sizeof(from);
        int n = socket_recvfrom(l, first, sizeof(first) - 1, 0, &from, &from_len);
        if (n >= 0) {
            first[n] = '\0';
            ui_color(UI_ACCENT);
            ui_printf("%s", (char *)first);
            ui_reset();
            socket_connect(l, &from, from_len);
            l->nonblock = 1;
            chat(l);
        }
        socket_close(l);
        return;
    }
    socket_t *conn;
    r = socket_accept(l, &conn);                // sleeps until someone connects
    socket_close(l);                            // one connection is all nc takes
    if (r < 0) {
        print_error("accept", r);
        return;
    }
    char text[INET_ADDR_TEXT];
    inet_format(conn->remote_ip, text);
    ui_printf("Connection received on %s %u\n", text, conn->remote_port);
    conn->nonblock = 1;
    chat(conn);
    socket_close(conn);
}

// -z: try each port: connect, then close at once.
static void scan(const options_t *o, uint32_t ip) {
    for (int port = o->port; port <= o->port_last && !signal_interrupted(); port++) {
        socket_t *s;
        if (socket_create(AF_INET, SOCK_STREAM, 0, &s) < 0)
            return;
        socket_timeval_t wait = { CONNECT_WAIT_MS / 1000, 0 };
        socket_setopt(s, SOL_SOCKET, SO_SNDTIMEO, &wait, sizeof(wait));
        sockaddr_t to;
        uint32_t to_len = sizeof(to);
        socket_inet_addr(&to, &to_len, ip, (uint16_t)port);
        int r = socket_connect(s, &to, to_len);
        if (r == 0) {
            ui_color(UI_GOOD);
            ui_printf("Connection to %s %d port [tcp] succeeded!\n", o->host, port);
            ui_reset();
        } else if (o->verbose) {
            ui_printf("nc: connect to %s port %d (tcp) failed: %s\n", o->host, port,
                      r == -NET_ECONNREFUSED ? "Connection refused" : "timed out");
        }
        socket_close(s);
    }
}

void cmd_nc(const char *args) {
    options_t o = { 0 };
    char word[DNS_NAME_MAX];
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-l") == 0)      o.listen = 1;
        else if (strcmp(word, "-u") == 0) o.udp = 1;
        else if (strcmp(word, "-z") == 0) o.scan = 1;
        else if (strcmp(word, "-v") == 0) o.verbose = 1;
        else if (strcmp(word, "-p") == 0) continue;     // "nc -l -p 1234" means the same
        else if (!o.listen && !o.host[0]) strcpy(o.host, word);    // the first word: the host
        else {
            o.port = atoi(word);                // "20" or "20-25"
            o.port_last = o.port;
            for (char *c = word; *c; c++)
                if (*c == '-')
                    o.port_last = atoi(c + 1);
        }
    }
    if (o.port <= 0 || o.port > 65535 || o.port_last < o.port || o.port_last > 65535 ||
        (!o.listen && !o.host[0])) {
        ui_usage(USAGE);
        return;
    }
    if (o.listen) {
        server(&o);
        return;
    }
    uint32_t ip;
    int rcode = dns_resolve(o.host, &ip);
    if (rcode != DNS_OK) {
        ui_start(UI_ERROR, "nc");
        ui_printf("%s: %s", o.host, dns_error_text(rcode));
        ui_end();
        return;
    }
    if (o.scan)
        scan(&o, ip);
    else
        client(&o, ip);
}
