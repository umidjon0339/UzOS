/*
 * cmd_wget.c - `wget URL`: download into a file.
 *
 *   wget http://example.com/
 *   Resolving example.com... 172.66.147.243
 *   Connecting to example.com|172.66.147.243|:80... connected.
 *   HTTP request sent, awaiting response... 200 OK
 *   Length: 1256 [text/html]
 *   Saving to: 'index.html'
 *   'index.html' saved [1256/1256]
 *
 *   wget -O page.html http://example.com/      choose the file name
 *
 * The file name is the URL's last part (index.html if it ends in '/').
 * The whole answer is read first, then written through open()/write(),
 * so the usual file permissions apply. A RAM disk file holds at most
 * 64 KiB.
 *
 * Uses: socket_* (core/socket), dns_resolve (core/dns), inet_format
 *       (core/inet), sys_open / sys_write / sys_close (io_management),
 *       signal_interrupted (process_management), arg_next, atoi, format,
 *       strcmp, strncmp, ui_*.
 */

#include "modules/network_management/commands/wget/cmd_wget.h"      // this command
#include "modules/network_management/core/socket/socket.h"          // socket_*
#include "modules/network_management/core/dns/dns.h"                // dns_resolve
#include "modules/network_management/core/inet/inet.h"              // inet_format, NET_E*
#include "modules/io_management/syscalls/open/sys_open.h"           // sys_open, O_WRITE, O_CREATE, O_TRUNC
#include "modules/io_management/syscalls/write/sys_write.h"         // sys_write
#include "modules/io_management/syscalls/close/sys_close.h"         // sys_close
#include "modules/io_management/core/fd/fd.h"                       // IO_MAX_TRANSFER
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"     // RAMDISK_MAX_FILE_SIZE
#include "modules/process_management/core/signal/signal.h"          // signal_interrupted
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/format/format.h"                                      // format
#include "lib/string/string.h"                                      // strcmp, strncmp, strcpy
#include "modules/ui/core/style/ui.h"                               // ui_*

#define USAGE     "wget [-O file] http://host[:port]/path"
#define URL_MAX   200
#define TIMEOUT_S 10

typedef struct {
    char host[DNS_NAME_MAX];
    int port;
    char path[URL_MAX];
} url_t;

static char response[RAMDISK_MAX_FILE_SIZE + 2048];   // headers + the biggest file

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

// The value of header 'name' in the headers (up to 'end'), or NULL.
static const char *header(const char *headers, const char *end, const char *name) {
    int len = (int)strlen(name);
    for (const char *line = headers; line < end; ) {
        int match = 1;
        for (int i = 0; i < len; i++)
            if ((line[i] | 32) != (name[i] | 32))   // case-insensitive
                match = 0;
        if (match && line[len] == ':') {
            const char *v = line + len + 1;
            while (*v == ' ')
                v++;
            return v;
        }
        while (line < end && *line != '\n')
            line++;
        line++;
    }
    return 0;
}

// The file name: -O, else the path's last part, else index.html.
static void file_name(const char *path, char *out, int max) {
    const char *last = path;
    for (const char *c = path; *c && *c != '?'; c++)
        if (*c == '/')
            last = c + 1;
    int n = 0;
    while (last[n] && last[n] != '?' && n < max - 1) {
        out[n] = last[n];
        n++;
    }
    out[n] = '\0';
    if (!n)
        strcpy(out, "index.html");
}

void cmd_wget(const char *args) {
    char word[URL_MAX], url_text[URL_MAX] = "", name[FS_NAME_MAX + 1] = "";
    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-O") == 0)
            rest = arg_next(rest, name, sizeof(name));
        else
            strcpy(url_text, word);
    }
    url_t url;
    int ok = parse_url(url_text, &url);
    if (ok <= 0) {
        if (ok < 0)
            ui_error("wget", "https:// needs TLS encryption, which UzOS does not have");
        else
            ui_usage(USAGE);
        return;
    }
    if (!name[0])
        file_name(url.path, name, sizeof(name));

    ui_printf("Resolving %s... ", url.host);
    uint32_t ip;
    int rcode = dns_resolve(url.host, &ip);
    if (rcode != DNS_OK) {
        ui_printf("failed: %s\n", dns_error_text(rcode));
        return;
    }
    char addr[INET_ADDR_TEXT];
    inet_format(ip, addr);
    ui_printf("%s\nConnecting to %s|%s|:%d... ", addr, url.host, addr, url.port);

    socket_t *s;
    if (socket_create(AF_INET, SOCK_STREAM, 0, &s) < 0) {
        ui_printf("failed: no socket\n");
        return;
    }
    socket_timeval_t timeout = { TIMEOUT_S, 0 };
    socket_setopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    socket_setopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    sockaddr_t to;
    uint32_t to_len = sizeof(to);
    socket_inet_addr(&to, &to_len, ip, (uint16_t)url.port);
    int r = socket_connect(s, &to, to_len);
    if (r < 0) {
        ui_printf("failed: %s.\n", r == -NET_ECONNREFUSED ? "Connection refused" : "Timed out");
        socket_close(s);
        return;
    }
    ui_printf("connected.\n");

    char request[URL_MAX + DNS_NAME_MAX + 80];
    int len = format(request, sizeof(request),
                     "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Wget/UzOS\r\n\r\n", url.path, url.host);
    socket_sendto(s, request, (uint32_t)len, 0, 0, 0);
    ui_printf("HTTP request sent, awaiting response... ");

    // Everything, until the server closes (or the buffer is full).
    int total = 0;
    while (!signal_interrupted() && total < (int)sizeof(response) - 1 &&
           (r = socket_recvfrom(s, response + total, sizeof(response) - 1 - total, 0, 0, 0)) > 0)
        total += r;
    socket_close(s);
    response[total] = '\0';

    // Status line "HTTP/1.0 200 OK", then headers, then an empty line.
    char *body = 0;
    for (int i = 0; i + 3 < total && !body; i++)
        if (response[i] == '\r' && response[i + 1] == '\n' && response[i + 2] == '\r' && response[i + 3] == '\n')
            body = response + i + 4;
    if (strncmp(response, "HTTP/", 5) != 0 || !body) {
        ui_printf("no valid HTTP answer.\n");
        return;
    }
    const char *status = response;
    while (*status && *status != ' ')
        status++;
    int code = atoi(status + 1);
    for (const char *c = status + 1; *c && *c != '\r'; c++)
        ui_printf("%c", *c);
    ui_printf("\n");
    if (code != 200) {
        ui_error("wget", "ERROR: the server did not send the file");
        return;
    }
    const char *type = header(response, body, "Content-Type");
    int body_len = total - (int)(body - response);
    ui_printf("Length: %d [", body_len);
    for (const char *c = type; c && *c && *c != '\r' && *c != ';'; c++)
        ui_printf("%c", *c);
    ui_printf("]\nSaving to: '%s'\n", name);

    if (body_len > RAMDISK_MAX_FILE_SIZE) {
        ui_error("wget", "too big for a RAM disk file (64 KiB)");
        return;
    }
    int fd = sys_open(name, O_WRITE | O_CREATE | O_TRUNC);
    if (fd < 0) {
        ui_error("wget", "cannot create the file (permission denied?)");
        return;
    }
    // One write() moves at most IO_MAX_TRANSFER bytes: write in pieces.
    for (int done = 0; done < body_len; ) {
        uint32_t left = (uint32_t)(body_len - done);
        uint32_t piece = left < IO_MAX_TRANSFER ? left : IO_MAX_TRANSFER;
        int n = sys_write(fd, body + done, piece);
        if (n <= 0)
            break;
        done += n;
    }
    sys_close(fd);
    ui_color(UI_GOOD);
    ui_printf("'%s' saved [%d/%d]\n", name, body_len, body_len);
    ui_reset();
}
