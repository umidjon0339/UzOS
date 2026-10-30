/*
 * cmd_httpd.c - `httpd`: a web server that runs in the background.
 *
 *   httpd start          serve /var/www on port 80 (a process "httpd")
 *   httpd start 8000     on another port
 *   httpd status         running? requests served, the last few
 *   httpd stop
 *
 * Try it:  curl http://localhost/           (from UzOS itself)
 *          http://localhost:8080/           (a browser on the host: QEMU
 *                                            forwards 8080 to our port 80)
 *
 * The server is the classic loop:
 *
 *   socket -> bind(port) -> listen -> accept -> read the request ->
 *   send the file -> close -> accept the next one ...
 *
 * one connection at a time. A request may not name a path with ".."
 * in it (that would climb out of /var/www), and a client gets 5 s for
 * its WHOLE request, however slowly it trickles in (DEFENCE
 * idle_timeout: a client that never finishes would otherwise block
 * everyone, security/slowloris).
 *
 * Uses: socket_* (core/socket), inet_format (core/inet), hostname_get
 *       (core/hostname), process_create / process_get_by_pid
 *       (process_management), vfs_lookup / vfs_read (filesystem_management),
 *       sys_mkdir / sys_open / sys_write / sys_close, sys_sleep, format, ui_*.
 */

#include "modules/network_management/commands/httpd/cmd_httpd.h"    // this command
#include "modules/network_management/core/socket/socket.h"          // socket_*
#include "modules/network_management/core/inet/inet.h"              // inet_format, NET_E*
#include "modules/network_management/core/hostname/hostname.h"      // hostname_get
#include "modules/process_management/core/process/process.h"        // process_create, process_caller_uid
#include "modules/process_management/syscalls/sleep/sys_sleep.h"    // sys_sleep
#include "modules/filesystem_management/core/vfs/vfs.h"             // vfs_lookup, vfs_read, fs_root
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"     // RAMDISK_MAX_FILE_SIZE
#include "modules/filesystem_management/syscalls/mkdir/sys_mkdir.h" // sys_mkdir
#include "modules/io_management/syscalls/open/sys_open.h"           // sys_open, O_WRITE, O_CREATE
#include "modules/io_management/syscalls/write/sys_write.h"         // sys_write
#include "modules/io_management/syscalls/close/sys_close.h"         // sys_close
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // atoi
#include "lib/format/format.h"                                      // format
#include "lib/string/string.h"                                      // strcmp, strncmp, strlen, strcpy
#include "modules/ui/core/style/ui.h"                               // ui_*
#include "modules/network_management/core/net_tick/net_tick.h"      // net_now_ms
#include "modules/network_management/security/slowloris/defence/idle_timeout/def_idle_timeout.h"  // def_request_time_ok

#define DOCROOT          "/var/www"
#define REQUEST_MAX      1024
#define READ_WAIT_S      1             // one read waits this long, then the deadline is checked
#define ACCEPT_SLICE_MS  500           // how often the loop checks "stop?"
#define LOG_LINES        5

static struct {
    int pid;                            // the daemon (0 = not running)
    int port;
    volatile int stop;                  // httpd stop asks it to end
    int error;                          // why it could not start
    uint32_t served;
    char log[LOG_LINES][80];            // the last requests
    int log_next;
} state;

static uint8_t file_buf[RAMDISK_MAX_FILE_SIZE];

/* ---------- Answering one request ---------- */

static const char *content_type(const char *path) {
    static const struct { const char *ext, *type; } types[] = {
        { ".html", "text/html" }, { ".htm", "text/html" }, { ".txt", "text/plain" },
        { ".css", "text/css" }, { ".js", "text/javascript" }, { ".png", "image/png" },
        { ".jpg", "image/jpeg" }, { ".json", "application/json" },
    };
    const char *dot = strrchr(path, '.');
    for (unsigned i = 0; dot && i < sizeof(types) / sizeof(types[0]); i++)
        if (strcmp(dot, types[i].ext) == 0)
            return types[i].type;
    return "application/octet-stream";
}

static void respond(socket_t *c, int code, const char *reason, const char *type,
                    const void *body, uint32_t len, int head_only) {
    char header[256];
    int n = format(header, sizeof(header),
                   "HTTP/1.0 %d %s\r\nServer: UzOS-httpd\r\nContent-Type: %s\r\n"
                   "Content-Length: %u\r\nConnection: close\r\n\r\n", code, reason, type, len);
    socket_sendto(c, header, (uint32_t)n, 0, 0, 0);
    if (!head_only && len)
        socket_sendto(c, body, len, 0, 0, 0);
}

static void error_page(socket_t *c, int code, const char *reason) {
    char body[128];
    int n = format(body, sizeof(body), "<h1>%d %s</h1>\n", code, reason);
    respond(c, code, reason, "text/html", body, (uint32_t)n, 0);
}

static void note(const socket_t *c, const char *method, const char *path, int code) {
    char ip[INET_ADDR_TEXT];
    inet_format(c->remote_ip, ip);
    format(state.log[state.log_next], sizeof(state.log[0]), "%s %s %s %d", ip, method, path, code);
    state.log_next = (state.log_next + 1) % LOG_LINES;
    state.served++;
}

static void serve(socket_t *c) {
    static char request[REQUEST_MAX + 1];
    socket_timeval_t wait = { READ_WAIT_S, 0 };
    socket_setopt(c, SOL_SOCKET, SO_RCVTIMEO, &wait, sizeof(wait));

    // Read until the empty line that ends the request (or the limit),
    // within the deadline for the whole request.
    uint32_t started = net_now_ms();
    int len = 0, done = 0;
    while (!done && len < REQUEST_MAX && def_request_time_ok(started)) {
        int r = socket_recvfrom(c, request + len, REQUEST_MAX - len, 0, 0, 0);
        if (r == -NET_EAGAIN)
            continue;                       // a quiet second: check the deadline, read again
        if (r <= 0)
            break;                          // closed, or broken
        len += r;
        request[len] = '\0';
        for (int i = 0; i + 3 < len && !done; i++)
            done = request[i] == '\r' && request[i + 1] == '\n' && request[i + 2] == '\r' && request[i + 3] == '\n';
    }
    if (!done)
        return;                             // too slow, too long, or gone: drop it

    // "GET /path HTTP/1.0"
    char method[8], path[FS_PATH_MAX];
    const char *rest = arg_next(request, method, sizeof(method));
    arg_next(rest, path, sizeof(path) - 16);
    int head_only = strcmp(method, "HEAD") == 0;
    if (strcmp(method, "GET") != 0 && !head_only) {
        note(c, method, path, 405);
        error_page(c, 405, "Method Not Allowed");
        return;
    }
    // Never above the document root.
    for (const char *p = path; *p; p++) {
        if (p[0] == '.' && p[1] == '.') {
            note(c, method, path, 403);
            error_page(c, 403, "Forbidden");
            return;
        }
    }
    char file[FS_PATH_MAX + 16];
    format(file, sizeof(file), "%s%s%s", DOCROOT, path, path[strlen(path) - 1] == '/' ? "index.html" : "");
    vfs_node_t *node = path[0] == '/' ? vfs_lookup(fs_root, file) : 0;
    if (!node || !(node->flags & FS_FILE)) {
        note(c, method, path, 404);
        error_page(c, 404, "Not Found");
        return;
    }
    uint32_t size = vfs_read(node, 0, sizeof(file_buf), file_buf);
    note(c, method, path, 200);
    respond(c, 200, "OK", content_type(file), file_buf, size, head_only);
}

/* ---------- The daemon ---------- */

static void httpd_main(void) {
    socket_t *l;
    int r = socket_create(AF_INET, SOCK_STREAM, 0, &l);
    if (r == 0) {
        int one = 1;
        socket_setopt(l, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        sockaddr_t me;
        uint32_t me_len = sizeof(me);
        socket_inet_addr(&me, &me_len, 0, (uint16_t)state.port);
        if ((r = socket_bind(l, &me, me_len)) == 0)
            r = socket_listen(l, 8);
        if (r < 0)
            socket_close(l);
    }
    if (r < 0) {
        state.error = r;
        state.pid = 0;
        return;                             // the process ends here
    }
    l->rcvtimeo_ms = ACCEPT_SLICE_MS;       // accept wakes up to check "stop?"
    while (!state.stop) {
        socket_t *c;
        r = socket_accept(l, &c);
        if (r == 0) {
            serve(c);
            socket_close(c);                // FIN: "that was all"
        } else if (r != -NET_EAGAIN && r != -NET_EINTR) {
            break;
        }
    }
    socket_close(l);
    state.pid = 0;
}

/* ---------- The command ---------- */

// /var/www/index.html, unless someone wrote their own.
static void make_docroot(void) {
    sys_mkdir("/var");
    sys_mkdir(DOCROOT);
    if (vfs_lookup(fs_root, DOCROOT "/index.html"))
        return;
    char page[512];
    int n = format(page, sizeof(page),
                   "<!DOCTYPE html>\n<html><head><title>UzOS</title></head>\n<body>\n"
                   "<h1>Hello from UzOS</h1>\n<p>This page came from <b>%s</b>, over the "
                   "UzOS network stack: e1000 -> Ethernet -> IPv4 -> TCP -> httpd.</p>\n"
                   "<p>Edit /var/www/index.html to change it.</p>\n</body></html>\n", hostname_get());
    int fd = sys_open(DOCROOT "/index.html", O_WRITE | O_CREATE);
    if (fd >= 0) {
        sys_write(fd, page, (uint32_t)n);
        sys_close(fd);
    }
}

static void start(int port) {
    if (state.pid) {
        ui_warn("httpd", "already running (httpd stop first)");
        return;
    }
    make_docroot();
    state.port = port;
    state.stop = 0;
    state.error = 0;
    int pid = process_create("httpd", httpd_main, process_caller_uid());
    if (pid < 0) {
        ui_error("httpd", "no free process slot");
        return;
    }
    state.pid = pid;
    sys_sleep(100);                         // let it try to bind
    if (state.error) {
        ui_start(UI_ERROR, "httpd");
        ui_printf("cannot listen on port %d: %s", port,
                  state.error == -NET_EADDRINUSE ? "Address already in use" :
                  state.error == -NET_EACCES ? "Permission denied (below 1024 needs CAP_NET_BIND)" : "error");
        ui_end();
        return;
    }
    ui_printf("httpd: serving %s on port %d (pid %d)\n", DOCROOT, port, pid);
}

static void stop(void) {
    if (!state.pid) {
        ui_warn("httpd", "not running");
        return;
    }
    state.stop = 1;
    for (int i = 0; i < 40 && state.pid; i++)
        sys_sleep(50);                      // it notices within ACCEPT_SLICE_MS
    ui_printf("httpd: stopped\n");
}

static void status(void) {
    if (!state.pid) {
        ui_printf("httpd: not running\n");
        return;
    }
    ui_printf("httpd: running, pid %d, port %d, %u requests served\n", state.pid, state.port, state.served);
    for (int i = 0; i < LOG_LINES; i++) {
        const char *line = state.log[(state.log_next + i) % LOG_LINES];
        if (line[0])
            ui_printf("  %s\n", line);
    }
}

void cmd_httpd(const char *args) {
    char word[8], port_text[8];
    const char *rest = arg_next(args, word, sizeof(word));
    arg_next(rest, port_text, sizeof(port_text));
    if (strcmp(word, "start") == 0) {
        int port = port_text[0] ? atoi(port_text) : 80;
        if (port <= 0 || port > 65535)
            ui_usage("httpd start [port]");
        else
            start(port);
    } else if (strcmp(word, "stop") == 0) {
        stop();
    } else if (!word[0] || strcmp(word, "status") == 0) {
        status();
    } else {
        ui_usage("httpd start [port] | stop | status");
    }
}
