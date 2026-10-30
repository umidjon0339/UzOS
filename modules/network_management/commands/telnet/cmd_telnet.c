/*
 * cmd_telnet.c - `telnet host [port]`: type to a server, read its answers.
 *
 *   telnet towel.blinkenlights.nl        (port 23: telnet itself)
 *   telnet example.com 80                any text protocol by hand
 *
 * Like nc, but it speaks the TELNET protocol: lines end in CR LF, and
 * byte 255 (IAC, "interpret as command") starts a negotiation:
 *
 *   IAC DO   <option>   "please turn <option> on"   -> we answer IAC WONT
 *   IAC WILL <option>   "I will turn it on"         -> we answer IAC DONT
 *
 * Refusing every option leaves the plainest session: text both ways.
 *
 * Everything you type goes over the wire AS IS, passwords too: anyone
 * on the path can read it (security/plaintext_sniffing). SSH replaced
 * telnet for exactly this reason.
 *
 * Uses: socket_* (core/socket), dns_resolve (core/dns), inet_format
 *       (core/inet), keyboard_pop_key (device_management),
 *       sys_sleep_interruptible / signal_interrupted (process_management),
 *       arg_next, atoi, ui_*.
 */

#include "modules/network_management/commands/telnet/cmd_telnet.h"  // this command
#include "modules/network_management/core/socket/socket.h"          // socket_*
#include "modules/network_management/core/dns/dns.h"                // dns_resolve
#include "modules/network_management/core/inet/inet.h"              // inet_format, NET_E*
#include "modules/device_management/keyboard/keyboard.h"            // keyboard_pop_key
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep_interruptible
#include "modules/process_management/core/signal/signal.h"          // signal_interrupted
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "modules/ui/core/style/ui.h"                               // ui_*

#define TELNET_PORT 23
#define IAC   255               // the next byte is a command
#define DONT  254
#define DO    253
#define WONT  252
#define WILL  251
#define SB    250               // sub-negotiation ... until IAC SE
#define SE    240
#define STEP_MS  10
#define LINE_MAX 200

// What the server's bytes are in the middle of.
typedef enum { DATA, GOT_IAC, GOT_VERB, IN_SB, IN_SB_IAC } parse_t;

// Show the server's bytes, answering every negotiation with "no".
static void receive(socket_t *s, parse_t *state, uint8_t *verb) {
    static uint8_t buf[512];
    int n;
    while ((n = socket_recvfrom(s, buf, sizeof(buf), MSG_DONTWAIT, 0, 0)) > 0) {
        for (int i = 0; i < n; i++) {
            uint8_t b = buf[i];
            switch (*state) {
            case DATA:
                if (b == IAC)
                    *state = GOT_IAC;
                else if (b != '\r')
                    ui_printf("%c", b);
                break;
            case GOT_IAC:
                if (b == DO || b == DONT || b == WILL || b == WONT) {
                    *verb = b;
                    *state = GOT_VERB;
                } else if (b == SB) {
                    *state = IN_SB;
                } else {
                    if (b == IAC)
                        ui_printf("%c", b);     // IAC IAC = a real byte 255
                    *state = DATA;
                }
                break;
            case GOT_VERB: {
                // DO x -> WONT x; WILL x -> DONT x; the others need no answer.
                uint8_t answer[3] = { IAC, *verb == DO ? WONT : DONT, b };
                if (*verb == DO || *verb == WILL)
                    socket_sendto(s, answer, 3, 0, 0, 0);
                *state = DATA;
                break;
            }
            case IN_SB:
                if (b == IAC)
                    *state = IN_SB_IAC;
                break;
            case IN_SB_IAC:
                *state = b == SE ? DATA : IN_SB;
                break;
            }
        }
    }
}

void cmd_telnet(const char *args) {
    char host[DNS_NAME_MAX], port_text[8];
    const char *rest = arg_next(args, host, sizeof(host));
    arg_next(rest, port_text, sizeof(port_text));
    int port = port_text[0] ? atoi(port_text) : TELNET_PORT;
    if (!host[0] || port <= 0 || port > 65535) {
        ui_usage("telnet host [port]");
        return;
    }
    uint32_t ip;
    int rcode = dns_resolve(host, &ip);
    if (rcode != DNS_OK) {
        ui_start(UI_ERROR, "telnet");
        ui_printf("%s: %s", host, dns_error_text(rcode));
        ui_end();
        return;
    }
    char addr[INET_ADDR_TEXT];
    inet_format(ip, addr);
    ui_printf("Trying %s...\n", addr);

    socket_t *s;
    if (socket_create(AF_INET, SOCK_STREAM, 0, &s) < 0)
        return;
    sockaddr_t to;
    uint32_t to_len = sizeof(to);
    socket_inet_addr(&to, &to_len, ip, (uint16_t)port);
    int r = socket_connect(s, &to, to_len);
    if (r < 0) {
        ui_error("telnet", r == -NET_ECONNREFUSED ? "Unable to connect: Connection refused"
                                                  : "Unable to connect: timed out");
        socket_close(s);
        return;
    }
    ui_printf("Connected to %s.\n", host);
    ui_hint("Ctrl+C closes the connection.");
    s->nonblock = 1;

    static char line[LINE_MAX];
    int len = 0;
    parse_t state = DATA;
    uint8_t verb = 0;
    while (!signal_interrupted()) {
        receive(s, &state, &verb);
        if (socket_poll(s) & POLLHUP) {
            ui_printf("\nConnection closed by foreign host.\n");
            break;
        }
        char key;
        while ((key = keyboard_pop_key()) != 0) {
            if (key == '\n') {
                ui_printf("\n");
                line[len++] = '\r';             // TELNET's end of line: CR LF
                line[len++] = '\n';
                socket_sendto(s, line, (uint32_t)len, 0, 0, 0);
                len = 0;
            } else if (key == '\b' && len > 0) {
                len--;
                ui_printf("\b \b");
            } else if (key >= ' ' && key < 127 && len < LINE_MAX - 3) {
                line[len++] = key;
                ui_printf("%c", key);
            }
        }
        sys_sleep_interruptible(STEP_MS);
    }
    socket_close(s);
}
