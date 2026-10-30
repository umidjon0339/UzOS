#ifndef UNIX_H
#define UNIX_H

/*
 * unix.h - AF_UNIX sockets: two programs on the SAME machine talk.
 *
 * No addresses, no packets, no network card: a UNIX socket is named by
 * a path ("/tmp/chat.sock"), and the bytes go straight from one
 * socket's memory into the other's. It is how most local services talk
 * (X11, D-Bus, systemd, databases).
 *
 *   server: socket(AF_UNIX, SOCK_STREAM) -> bind("/tmp/s") -> listen -> accept
 *   client: socket(AF_UNIX, SOCK_STREAM) -> connect("/tmp/s")
 *   or:     socketpair(AF_UNIX, SOCK_STREAM, 0, fds)   two ends, already joined
 *
 * Because both ends are on this machine, the kernel KNOWS who is on the
 * other side: SO_PEERCRED tells the pid and uid of the peer, which a
 * TCP server could never trust (security/unix_impersonation).
 *
 * (Linux also makes a file of type "socket" at the path; UzOS keeps the
 * names in its own table.)
 */

#include <stdint.h>
#include "modules/network_management/core/socket/socket.h"     // socket_t, socket_ops_t

#define UNIX_PATH_MAX  108          // sun_path, as Linux
#define UNIX_MAX       16           // UNIX socket ends at once
#define UNIX_BUF_SIZE  4096         // stream bytes waiting per end
#define UNIX_BACKLOG   8

// Linux's struct sockaddr_un.
typedef struct {
    uint16_t family;                // AF_UNIX
    char path[UNIX_PATH_MAX];
} sockaddr_un_t;

typedef struct unix_sock {
    int used;
    socket_t *sock;                 // the socket this end belongs to
    char path[UNIX_PATH_MAX];       // bound name ("" = none)
    struct unix_sock *peer;         // the other end (stream, or connected dgram)
    socket_cred_t peer_cred;        // who the peer was when it connected

    // Stream: bytes the peer wrote, waiting for THIS end to read.
    uint8_t buf[UNIX_BUF_SIZE];
    uint32_t head, len;
    int peer_gone;                  // the peer closed: after the bytes, EOF

    // A listener's queue of connections not accepted yet.
    struct unix_sock *queue[UNIX_BACKLOG];
    int queued, backlog;
} unix_sock_t;

extern const socket_ops_t socket_unix_ops;

// Join two new sockets to each other (socketpair). 0 or -NET_E*.
int unix_pair(socket_t *a, socket_t *b);

// SO_PEERCRED: who was at the other end when it connected. 0 or -NET_ENOTCONN.
int unix_peer_cred(socket_t *s, socket_cred_t *out);

const unix_sock_t *unix_get(int slot);      // for netstat -x / ss -x

#endif
