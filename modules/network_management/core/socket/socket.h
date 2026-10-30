#ifndef SOCKET_H
#define SOCKET_H

/*
 * socket.h - Sockets: a program's end of a conversation over the network.
 *
 *   int fd = socket(AF_INET, SOCK_DGRAM, 0);        a UDP socket
 *   sendto(fd, "hi", 2, 0, &server, sizeof(server));
 *   recvfrom(fd, buf, 512, 0, &from, &fromlen);
 *   close(fd);
 *
 * A socket is a FILE DESCRIPTOR like an open file: read(), write() and
 * close() work on it too. Inside, it is a socket_t whose first member
 * is a vfs_node_t, so the descriptor table (process_t.fd_table) can
 * point at it like at any file ("sockfs", as Linux calls it).
 *
 * What a socket does depends on its family and type:
 *
 *   AF_INET + SOCK_DGRAM    UDP        sock_udp.c
 *   AF_INET + SOCK_STREAM   TCP        sock_tcp.c
 *   AF_INET + SOCK_RAW      raw IP     sock_raw.c
 *   AF_UNIX                 local      core/unix
 *
 * Each kind fills a socket_ops_t; socket.c does the common work: the
 * table, descriptors, addresses, and WAITING. An operation that cannot
 * finish yet returns -NET_EAGAIN; then, unless the socket is
 * non-blocking, socket.c puts the process to sleep until the network
 * wakes it (socket_wake) and tries again.
 */

#include <stdint.h>
#include "modules/filesystem_management/core/vfs/vfs.h"         // vfs_node_t
#include "modules/network_management/core/packet/packet.h"      // packet_queue_t
#include "modules/network_management/core/udp/udp.h"            // udp_endpoint_t
#include "modules/process_management/core/process/process.h"    // process_t

#define SOCKET_MAX 32                   // sockets open at once, all programs together

// Families
#define AF_UNSPEC  0
#define AF_UNIX    1                    // on this machine, named by a path
#define AF_INET    2                    // IPv4
// Types (the high bits may carry flags, as in Linux)
#define SOCK_STREAM    1                // a connection: bytes in order (TCP)
#define SOCK_DGRAM     2                // separate messages (UDP)
#define SOCK_RAW       3                // whole IP packets (needs CAP_NET_RAW)
#define SOCK_TYPE_MASK 0xF
#define SOCK_NONBLOCK  0x800            // never sleep: -EAGAIN instead
#define SOCK_CLOEXEC   0x80000
// Protocols
#define IPPROTO_IP     0                // "the usual one for this type"

// setsockopt / getsockopt
#define SOL_SOCKET     1
#define SO_REUSEADDR   2
#define SO_TYPE        3
#define SO_ERROR       4
#define SO_BROADCAST   6
#define SO_SNDBUF      7
#define SO_RCVBUF      8
#define SO_KEEPALIVE   9
#define SO_REUSEPORT   15
#define SO_PEERCRED    17
#define SO_RCVTIMEO    20
#define SO_SNDTIMEO    21
#define SO_ACCEPTCONN  30
#define SOL_IP         0
#define IP_TTL         2
#define SOL_TCP        6
#define TCP_NODELAY    1
#define TCP_MAXSEG     2

// send / recv flags
#define MSG_PEEK       0x2              // look, but leave it in the queue
#define MSG_DONTWAIT   0x40             // this once: do not sleep
#define MSG_WAITALL    0x100            // stream: wait for the whole length

// shutdown(how)
#define SHUT_RD   0
#define SHUT_WR   1
#define SHUT_RDWR 2

// poll() bits (socket_poll answers with these)
#define POLLIN   0x001                  // something to read (or a connection to accept)
#define POLLPRI  0x002
#define POLLOUT  0x004                  // room to write
#define POLLERR  0x008
#define POLLHUP  0x010                  // the other side is gone
#define POLLNVAL 0x020

#define SOCKADDR_MAX 110                // the biggest address: sockaddr_un

// What a program passes in: the family first, then the address.
typedef struct {
    uint16_t family;
    uint8_t  data[SOCKADDR_MAX - 2];
} sockaddr_t;

// AF_INET: port and address in NETWORK byte order (as in Linux).
typedef struct {
    uint16_t family;                    // AF_INET
    uint16_t port;
    uint32_t addr;
    uint8_t  zero[8];
} sockaddr_in_t;

// Linux's struct timeval, for SO_RCVTIMEO.
typedef struct {
    uint32_t sec, usec;
} socket_timeval_t;

// SO_PEERCRED: who is at the other end of a local socket.
typedef struct {
    uint32_t pid, uid, gid;
} socket_cred_t;

typedef enum {
    SS_UNCONNECTED,                     // new, or a bound datagram socket
    SS_LISTENING,                       // listen(): waits for connections
    SS_CONNECTING,                      // connect() sent, no answer yet
    SS_CONNECTED,
    SS_CLOSED,                          // the conversation is over
} socket_state_t;

struct socket;

typedef struct socket_ops {
    // 0 or -NET_E*; -NET_EAGAIN = "not yet, wait"; -NET_EINPROGRESS = "started".
    int (*bind)(struct socket *s, const sockaddr_t *addr, uint32_t len);
    int (*connect)(struct socket *s, const sockaddr_t *addr, uint32_t len);
    int (*listen)(struct socket *s, int backlog);
    int (*accept)(struct socket *s, struct socket **child);
    // Bytes sent / received, or -NET_E*. 'addr' may be NULL.
    int (*send)(struct socket *s, const void *buf, uint32_t len, int flags,
                const sockaddr_t *addr, uint32_t addrlen);
    int (*recv)(struct socket *s, void *buf, uint32_t len, int flags,
                sockaddr_t *addr, uint32_t *addrlen);
    int (*shutdown)(struct socket *s, int how);
    // Our own / the peer's address (for getsockname / getpeername).
    int (*name)(struct socket *s, int peer, sockaddr_t *addr, uint32_t *len);
    int (*poll)(struct socket *s);      // POLL* bits ready now
    void (*close)(struct socket *s);    // the last descriptor went away
} socket_ops_t;

typedef struct socket {
    vfs_node_t node;                    // FIRST: a pointer to the node is a pointer to the socket
    int used;
    int family, type, protocol;
    const socket_ops_t *ops;
    socket_state_t state;
    uint32_t uid;                       // who made it
    uint32_t pid;
    int nonblock;                       // SOCK_NONBLOCK / FIONBIO
    int error;                          // SO_ERROR: an error that happened while nobody asked
    int shut_rd, shut_wr;               // shutdown() done
    process_t *waiter;                  // the process sleeping on it (socket_wake)

    // Options
    int reuseaddr, reuseport, broadcast, keepalive, nodelay;
    uint8_t ttl;                        // 0 = default
    uint32_t rcvbuf, sndbuf;            // limits, in bytes as charged
    uint32_t rcvtimeo_ms, sndtimeo_ms;  // 0 = wait forever

    // INET addresses (host order)
    uint32_t local_ip, remote_ip;
    uint16_t local_port, remote_port;

    // Received, not yet read (datagrams; the packet keeps its headers,
    // so recvfrom() can read who sent it).
    packet_queue_t rx;
    uint32_t rx_charged;                // bytes charged against rcvbuf
    uint32_t drops;                     // datagrams thrown away: rcvbuf full

    udp_endpoint_t udp;                 // SOCK_DGRAM: our port in UDP's table
    void *proto;                        // TCP connection / UNIX peer, ...
} socket_t;

// What one received packet costs against rcvbuf: the whole buffer, not
// just the bytes in it (Linux charges the "truesize" too).
#define SOCKET_TRUESIZE ((uint32_t)sizeof(packet_t))
#define SOCKET_RCVBUF_DEFAULT (16 * SOCKET_TRUESIZE)
#define SOCKET_SNDBUF_DEFAULT 16384

/* ---------- Making and finding sockets ---------- */

// A new socket. 0 and *out set, or -NET_E*. Checks the caller's
// capabilities (SOCK_RAW needs CAP_NET_RAW).
int socket_create(int family, int type, int protocol, socket_t **out);

// Put s into p's descriptor table: the descriptor, or -NET_EMFILE.
int socket_install_fd(process_t *p, socket_t *s);

// The socket behind descriptor fd of p, or NULL (not open / not a socket).
socket_t *socket_from_fd(process_t *p, int fd);
int socket_is(const vfs_node_t *node);

// Why socket_from_fd() said NULL: -NET_EBADF (no such descriptor) or
// -NET_ENOTSOCK (an open file, not a socket).
int socket_fd_error(process_t *p, int fd);

// Close a socket nobody has a descriptor for (kernel users, failed accept).
void socket_close(socket_t *s);

const socket_t *socket_get(int slot);   // slot 0 .. SOCKET_MAX-1, for netstat / ss

/* ---------- Using them (also for the shell's commands) ---------- */

int socket_bind(socket_t *s, const sockaddr_t *addr, uint32_t len);
int socket_connect(socket_t *s, const sockaddr_t *addr, uint32_t len);
int socket_listen(socket_t *s, int backlog);
int socket_accept(socket_t *s, socket_t **child);
int socket_sendto(socket_t *s, const void *buf, uint32_t len, int flags,
                  const sockaddr_t *addr, uint32_t addrlen);
int socket_recvfrom(socket_t *s, void *buf, uint32_t len, int flags,
                    sockaddr_t *addr, uint32_t *addrlen);
int socket_shutdown(socket_t *s, int how);
int socket_getname(socket_t *s, int peer, sockaddr_t *addr, uint32_t *len);
int socket_setopt(socket_t *s, int level, int name, const void *value, uint32_t len);
int socket_getopt(socket_t *s, int level, int name, void *value, uint32_t *len);
int socket_poll(socket_t *s);

// How many bytes a read would get now (ioctl FIONREAD): a stream's
// buffered bytes, or the size of the next datagram.
int socket_pending_bytes(socket_t *s);

/* ---------- For the protocols ---------- */

// Something changed (data arrived, a connection came, the peer left):
// wake the process sleeping on s. Safe in an interrupt.
void socket_wake(socket_t *s);

// Queue a received datagram (charged against rcvbuf). Takes p; 0 if
// the queue was full and p was dropped.
int socket_queue(socket_t *s, packet_t *p);

// Make a sockaddr_in from host-order ip and port.
void socket_inet_addr(sockaddr_t *out, uint32_t *len, uint32_t ip, uint16_t port);

// Read an AF_INET address (host order out). 0 or -NET_EINVAL / -NET_EAFNOSUPPORT.
int socket_inet_parse(const sockaddr_t *addr, uint32_t len, uint32_t *ip, uint16_t *port);

// The operations of each kind (sock_udp.c, sock_tcp.c, ...).
extern const socket_ops_t socket_udp_ops;
extern const socket_ops_t socket_tcp_ops;
extern const socket_ops_t socket_raw_ops;

// A received IP packet (p->data = its IP header): a copy for every raw
// socket of its protocol (ipv4_receive calls this).
void socket_raw_deliver(packet_t *p);

#endif
