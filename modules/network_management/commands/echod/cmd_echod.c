/*
 * cmd_echod.c - `echod`: the echo service (RFC 862), TCP and UDP.
 *
 *   echod start          port 7, both TCP and UDP (a process "echod")
 *   nc localhost 7       type a line: it comes back
 *   nc -u localhost 7    the same with datagrams
 *   echod stop
 *
 * The simplest server there is, which makes it a good test partner for
 * the stack. One process watches both sockets: all of them
 * non-blocking, one loop, a short nap when there is nothing to do. It
 * serves one TCP client at a time.
 *
 * Uses: socket_* (core/socket), process_create (process_management),
 *       sys_sleep (process_management), arg_next, atoi, strcmp, ui_*.
 */

#include "modules/network_management/commands/echod/cmd_echod.h"    // this command
#include "modules/network_management/core/socket/socket.h"          // socket_*
#include "modules/network_management/core/inet/inet.h"              // NET_E*
#include "modules/process_management/core/process/process.h"        // process_create, process_caller_uid
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/string/string.h"                                      // strcmp
#include "modules/ui/core/style/ui.h"                               // ui_*

#define ECHO_PORT 7
#define NAP_MS    10

static struct {
    int pid;
    int port;
    volatile int stop;
    int error;
    uint32_t tcp_bytes, udp_datagrams;
} state;

// A non-blocking socket of 'type' bound to the port, or NULL.
static socket_t *open_socket(int type) {
    socket_t *s;
    if (socket_create(AF_INET, type | SOCK_NONBLOCK, 0, &s) < 0)
        return 0;
    int one = 1;
    socket_setopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    sockaddr_t me;
    uint32_t me_len = sizeof(me);
    socket_inet_addr(&me, &me_len, 0, (uint16_t)state.port);
    int r = socket_bind(s, &me, me_len);
    if (r == 0 && type == SOCK_STREAM)
        r = socket_listen(s, 4);
    if (r < 0) {
        state.error = r;
        socket_close(s);
        return 0;
    }
    return s;
}

static void echod_main(void) {
    socket_t *udp = open_socket(SOCK_DGRAM);
    socket_t *listener = udp ? open_socket(SOCK_STREAM) : 0;
    socket_t *client = 0;
    static uint8_t buf[1024];
    while (listener && !state.stop) {
        sockaddr_t from;
        uint32_t from_len = sizeof(from);
        int n = socket_recvfrom(udp, buf, sizeof(buf), 0, &from, &from_len);
        if (n >= 0) {                           // a datagram: send it back to its sender
            socket_sendto(udp, buf, (uint32_t)n, 0, &from, from_len);
            state.udp_datagrams++;
        }
        if (!client && socket_accept(listener, &client) == 0)
            client->nonblock = 1;
        if (client) {
            n = socket_recvfrom(client, buf, sizeof(buf), 0, 0, 0);
            if (n > 0) {
                socket_sendto(client, buf, (uint32_t)n, 0, 0, 0);
                state.tcp_bytes += (uint32_t)n;
            } else if (n != -NET_EAGAIN) {          // 0: it hung up; < 0: it broke
                socket_close(client);
                client = 0;
            }
        }
        sys_sleep(NAP_MS);
    }
    if (client)
        socket_close(client);
    if (listener)
        socket_close(listener);
    if (udp)
        socket_close(udp);
    state.pid = 0;
}

void cmd_echod(const char *args) {
    char word[8], port_text[8];
    const char *rest = arg_next(args, word, sizeof(word));
    arg_next(rest, port_text, sizeof(port_text));

    if (strcmp(word, "start") == 0) {
        if (state.pid) {
            ui_warn("echod", "already running");
            return;
        }
        state.port = port_text[0] ? atoi(port_text) : ECHO_PORT;
        state.stop = 0;
        state.error = 0;
        int pid = process_create("echod", echod_main, process_caller_uid());
        if (pid < 0) {
            ui_error("echod", "no free process slot");
            return;
        }
        state.pid = pid;
        sys_sleep(100);                         // let it bind
        if (state.error)
            ui_error("echod", state.error == -NET_EADDRINUSE ? "port in use" : "cannot bind the port");
        else
            ui_printf("echod: echoing TCP and UDP on port %d (pid %d)\n", state.port, pid);
    } else if (strcmp(word, "stop") == 0) {
        state.stop = 1;
        for (int i = 0; i < 20 && state.pid; i++)
            sys_sleep(20);
        ui_printf("echod: stopped\n");
    } else if (!word[0] || strcmp(word, "status") == 0) {
        if (state.pid)
            ui_printf("echod: running, pid %d, port %d: %u TCP bytes, %u UDP datagrams echoed\n",
                      state.pid, state.port, state.tcp_bytes, state.udp_datagrams);
        else
            ui_printf("echod: not running\n");
    } else {
        ui_usage("echod start [port] | stop | status");
    }
}
