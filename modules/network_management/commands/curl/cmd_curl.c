/*
 * cmd_curl.c - `curl URL`: one HTTP request, the answer on the screen.
 *
 *   curl http://example.com/          the page
 *   curl -I http://example.com/       only the response headers
 *   curl -v http://localhost/         also the request and the headers
 *
 * HTTP is just text over a TCP connection:
 *
 *   GET / HTTP/1.0                    the request line: method, path, version
 *   Host: example.com                 headers ...
 *                                     an empty line: the end of the request
 *
 *   HTTP/1.0 200 OK                   the status line
 *   Content-Type: text/html           headers ...
 *                                     an empty line
 *   <html>...                         the body, until the server closes
 *
 * HTTP/1.0 is used on purpose: the server then sends the body plainly
 * and closes when done (1.1 may send it in "chunks"). https:// needs
 * TLS encryption, which UzOS does not have.
 *
 * Uses: socket_* (core/socket), dns_resolve (core/dns), inet_format
 *       (core/inet), signal_interrupted (process_management), arg_next,
 *       atoi, format, strcmp, strncmp, ui_*.
 */

#include "modules/network_management/commands/curl/cmd_curl.h"      // this command
#include "modules/network_management/core/socket/socket.h"          // socket_*
#include "modules/network_management/core/dns/dns.h"                // dns_resolve
#include "modules/network_management/core/inet/inet.h"              // inet_format, NET_E*
#include "modules/process_management/core/signal/signal.h"          // signal_interrupted
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/format/format.h"                                      // format
#include "lib/string/string.h"                                      // strcmp, strncmp, strlen
#include "modules/ui/core/style/ui.h"                               // ui_*

#define USAGE        "curl [-I] [-v] http://host[:port]/path"
#define URL_MAX      200
#define TIMEOUT_S    10

typedef struct {
    char host[DNS_NAME_MAX];
    int port;
    char path[URL_MAX];
} url_t;

// "http://host:port/path" -> its parts. 0 if it is not an http URL.
static int parse_url(const char *text, url_t *u) {
    if (strncmp(text, "https://", 8) == 0)
        return -1;
    if (strncmp(text, "http://", 7) == 0)
        text += 7;
    int n = 0;
    while (*text && *text != '/' && *text != ':' && n < DNS_NAME_MAX - 1)
        u->host[n++] = *text++;
    u->host[n] = '\0';
    u->port = 80;
    if (*text == ':') {
        u->port = atoi(++text);
        while (*text >= '0' && *text <= '9')
            text++;
    }
    n = 0;
    if (*text != '/')
        u->path[n++] = '/';
    while (*text && n < URL_MAX - 1)
        u->path[n++] = *text++;
    u->path[n] = '\0';
    return u->host[0] && u->port > 0 && u->port < 65536;
}

void cmd_curl(const char *args) {
    char word[URL_MAX], url_text[URL_MAX] = "";
    int head_only = 0, verbose = 0;
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-I") == 0)      head_only = 1;
        else if (strcmp(word, "-v") == 0) verbose = 1;
        else                              strcpy(url_text, word);
    }
    url_t url;
    int ok = parse_url(url_text, &url);
    if (ok < 0) {
        ui_error("curl", "https:// needs TLS encryption, which UzOS does not have: use http://");
        return;
    }
    if (!ok) {
        ui_usage(USAGE);
        return;
    }

    uint32_t ip;
    int rcode = dns_resolve(url.host, &ip);
    if (rcode != DNS_OK) {
        ui_start(UI_ERROR, "curl");
        ui_printf("(6) Could not resolve host: %s", url.host);
        ui_end();
        return;
    }
    socket_t *s;
    if (socket_create(AF_INET, SOCK_STREAM, 0, &s) < 0) {
        ui_error("curl", "no socket");
        return;
    }
    socket_timeval_t timeout = { TIMEOUT_S, 0 };
    socket_setopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    socket_setopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    sockaddr_t to;
    uint32_t to_len = sizeof(to);
    socket_inet_addr(&to, &to_len, ip, (uint16_t)url.port);
    char addr[INET_ADDR_TEXT];
    inet_format(ip, addr);
    if (verbose)
        ui_printf("*   Trying %s:%d...\n", addr, url.port);
    int r = socket_connect(s, &to, to_len);
    if (r < 0) {
        ui_start(UI_ERROR, "curl");
        ui_printf("(7) Failed to connect to %s port %d: %s", url.host, url.port,
                  r == -NET_ECONNREFUSED ? "Connection refused" : "Timed out");
        ui_end();
        socket_close(s);
        return;
    }

    static char request[URL_MAX + DNS_NAME_MAX + 100];
    int len = format(request, sizeof(request),
                     "%s %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: curl/UzOS\r\nAccept: */*\r\n\r\n",
                     head_only ? "HEAD" : "GET", url.path, url.host);
    if (verbose) {
        ui_printf("* Connected to %s (%s) port %d\n", url.host, addr, url.port);
        ui_color(UI_DIM);
        ui_printf("> %s %s HTTP/1.0\n> Host: %s\n>\n", head_only ? "HEAD" : "GET", url.path, url.host);
        ui_reset();
    }
    socket_sendto(s, request, (uint32_t)len, 0, 0, 0);

    // Read until the server closes. The headers end at the first empty
    // line; -I and -v show them ("< " in front, as curl -v does).
    static char buf[513];
    int in_headers = 1, new_line = 1, total = 0;
    int show_headers = head_only || verbose;
    while (!signal_interrupted() && (r = socket_recvfrom(s, buf, sizeof(buf) - 1, 0, 0, 0)) > 0) {
        for (int i = 0; i < r; i++) {
            char c = buf[i];
            if (!in_headers) {
                if (!head_only) {
                    ui_printf("%c", c);
                    total++;
                }
                continue;
            }
            if (c == '\r')
                continue;                       // lines end in CR LF: keep only LF
            if (c == '\n' && new_line) {        // an empty line: the body begins
                in_headers = 0;
                if (show_headers)
                    ui_printf("\n");
                continue;
            }
            if (show_headers) {
                ui_color(UI_DIM);
                ui_printf(new_line && verbose ? "< %c" : "%c", c);
                ui_reset();
            }
            new_line = c == '\n';
        }
    }
    if (r < 0 && r != -NET_EINTR) {
        ui_start(UI_ERROR, "curl");
        ui_printf("(56) Failure receiving data (%s)", r == -NET_EAGAIN ? "timed out" : "reset");
        ui_end();
    }
    if (verbose)
        ui_printf("\n* Closing connection: %d bytes of body\n", total);
    socket_close(s);
}
