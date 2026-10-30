#ifndef INET_H
#define INET_H

/*
 * inet.h - Byte order, and addresses as text.
 *
 * Byte order. The x86 CPU keeps numbers "little-endian" (lowest byte
 * first in memory); the network sends them "big-endian" (highest byte
 * first: "network byte order"). Port 80 = 0x0050:
 *
 *     in a uint16_t on x86:   50 00
 *     on the wire:            00 50
 *
 * So every number is turned around on its way into or out of a header:
 * htons = "host to network, short (16 bits)", ntohl = "network to host,
 * long (32 bits)", ...
 *
 * Inside UzOS an IPv4 address is a uint32_t in HOST order, so that
 * 10.0.2.15 is 0x0A00020F and "ip & mask" reads naturally. Headers and
 * the user's sockaddr_in hold it in network order; the code that reads
 * or writes them converts.
 */

#include <stdint.h>

#define INET_ADDR_TEXT 16       // "255.255.255.255" + NUL
#define MAC_TEXT       18       // "ff:ff:ff:ff:ff:ff" + NUL

static inline uint16_t htons(uint16_t x) { return (uint16_t)((x << 8) | (x >> 8)); }
static inline uint16_t ntohs(uint16_t x) { return htons(x); }
static inline uint32_t htonl(uint32_t x) {
    return (x << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24);
}
static inline uint32_t ntohl(uint32_t x) { return htonl(x); }

// Build an address from its four parts: INET_ADDR(10, 0, 2, 15).
#define INET_ADDR(a, b, c, d) \
    (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(c) << 8) | (uint32_t)(d))

// "10.0.2.15" -> 0x0A00020F. Returns 1, or 0 if the text is not an address.
int inet_parse(const char *text, uint32_t *ip);

// 0x0A00020F -> "10.0.2.15"
void inet_format(uint32_t ip, char out[INET_ADDR_TEXT]);

// A prefix length and its mask: 24 <-> 255.255.255.0
uint32_t inet_mask(int prefix);
int inet_prefix(uint32_t mask);

// 52:54:00:12:34:56
void mac_format(const uint8_t mac[6], char out[MAC_TEXT]);

/* ---------- The Internet checksum ----------
 * IP, ICMP, UDP and TCP all protect their bytes the same way: add them
 * up as 16-bit words, carry the overflow back in ("one's complement"),
 * and store the inverse. The receiver adds up everything INCLUDING the
 * checksum and must get 0xFFFF, so inet_checksum() over it gives 0.
 * The result can be stored into a header as it is (no htons). */

uint16_t inet_checksum(const void *data, uint32_t len);

// UDP and TCP also cover a "pseudo header": the IP addresses, the
// protocol and the length, so a packet delivered to the wrong machine
// fails the check too. src and dst in host order.
uint16_t inet_checksum_pseudo(uint32_t src, uint32_t dst, uint8_t protocol,
                              const void *data, uint32_t len);

/* ---------- Errors ----------
 * The stack returns Linux's error numbers, negative (-NET_ENETUNREACH),
 * so a socket syscall can hand them to the program unchanged. */

#define NET_EPERM         1     // not allowed (a capability is missing)
#define NET_EINTR         4     // a signal (Ctrl+C) ended the wait
#define NET_EBADF         9     // not an open descriptor
#define NET_EAGAIN        11    // nothing yet: try again
#define NET_ENOMEM        12
#define NET_EACCES        13    // permission denied
#define NET_EFAULT        14    // a bad user pointer
#define NET_EINVAL        22    // a bad argument
#define NET_EMFILE        24    // no free file descriptor
#define NET_EPIPE         32    // the other side closed
#define NET_ENOTSOCK      88    // that descriptor is not a socket
#define NET_EDESTADDRREQ  89
#define NET_EMSGSIZE      90    // too long for one datagram
#define NET_EPROTOTYPE    91
#define NET_ENOPROTOOPT   92
#define NET_EPROTONOSUPPORT 93
#define NET_EOPNOTSUPP    95
#define NET_EAFNOSUPPORT  97    // address family not supported
#define NET_EADDRINUSE    98    // the port is taken
#define NET_EADDRNOTAVAIL 99    // not one of our addresses
#define NET_ENETDOWN      100
#define NET_ENETUNREACH   101   // no route to that network
#define NET_ECONNABORTED  103
#define NET_ECONNRESET    104   // the other side reset the connection
#define NET_ENOBUFS       105   // no packet buffer free
#define NET_EISCONN       106
#define NET_ENOTCONN      107
#define NET_ETIMEDOUT     110
#define NET_ECONNREFUSED  111   // nobody listens on that port
#define NET_EHOSTUNREACH  113
#define NET_EALREADY      114
#define NET_EINPROGRESS   115

#endif
