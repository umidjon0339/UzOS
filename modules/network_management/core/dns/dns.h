#ifndef DNS_H
#define DNS_H

/*
 * dns.h - DNS: turn a name (example.com) into an address (93.184.215.14).
 *
 * The resolver looks in three places, in order:
 *   1. /etc/hosts              names fixed by hand ("127.0.0.1 localhost")
 *   2. its cache               answers it got recently (for their TTL)
 *   3. a DNS server            the one in /etc/resolv.conf, else DHCP's
 *
 * A question to the server is one UDP datagram to port 53, the answer
 * one datagram back:
 *
 *   header    id | flags (QR, RD, RA, rcode) | 1 question | N answers
 *   question  7example3com0 | type A (1) | class IN (1)
 *   answers   name | type | class | TTL | length | data (4 bytes for A)
 *
 * Names are "labels": a length byte, then that many letters. To save
 * room an answer may point back at a name earlier in the message
 * ("compression": two bytes 0xC0 | offset).
 *
 * The answer is accepted only if its id and question match ours and it
 * came from the server we asked. A forged answer that guesses them is
 * DNS spoofing (security/dns_spoofing).
 */

#include <stdint.h>

#define DNS_PORT        53
#define DNS_NAME_MAX    128
#define DNS_MAX_RECORDS 12
#define DNS_TIMEOUT_MS  2000
#define DNS_TRIES       2

// Record types
#define DNS_TYPE_A      1
#define DNS_TYPE_NS     2
#define DNS_TYPE_CNAME  5
#define DNS_TYPE_PTR    12
#define DNS_TYPE_MX     15
#define DNS_TYPE_TXT    16
#define DNS_TYPE_AAAA   28

// Answer codes (rcode), and our own failures (negative)
#define DNS_OK          0
#define DNS_FORMERR     1
#define DNS_SERVFAIL    2
#define DNS_NXDOMAIN    3       // "no such name"
#define DNS_REFUSED     5
#define DNS_ERR_TIMEOUT  -1     // the server never answered
#define DNS_ERR_NOSERVER -2     // no DNS server known
#define DNS_ERR_BADNAME  -3
#define DNS_ERR_NETWORK  -4     // could not send (no route, ...)
#define DNS_ERR_NODATA   -5     // the name exists, but has no record of that type

typedef struct {
    char name[DNS_NAME_MAX];        // whose record
    uint16_t type;
    uint32_t ttl;                   // seconds it may be cached
    uint32_t ip;                    // A: the address (host order)
    uint16_t preference;            // MX
    char text[DNS_NAME_MAX];        // CNAME / NS / PTR / MX: a name; TXT: the text; AAAA: hex
} dns_record_t;

typedef struct {
    int rcode;                      // DNS_OK, DNS_NXDOMAIN, ... or DNS_ERR_*
    uint16_t id;
    uint16_t flags;                 // the header's flag bits, for dig
    uint32_t server;                // who answered
    uint32_t time_ms;               // how long it took
    int count;                      // records in 'answers'
    dns_record_t answers[DNS_MAX_RECORDS];
} dns_result_t;

// Create /etc/hosts and /etc/resolv.conf if they are missing. Boot.
void dns_init(void);

// The DNS server: /etc/resolv.conf's "nameserver", else DHCP's, else 0.
uint32_t dns_nameserver(void);

// Write "nameserver <ip>" into /etc/resolv.conf (DHCP's answer).
void dns_set_nameserver(uint32_t ip);

// Ask 'server' (0 = dns_nameserver()) for records of 'type'. Always asks:
// no hosts file, no cache (that is what dig wants). Blocks.
int dns_query(const char *name, uint16_t type, uint32_t server, dns_result_t *out);

// name -> IPv4 address the normal way: numbers, /etc/hosts, cache, server.
// DNS_OK and *ip, or an error code. Blocks.
int dns_resolve(const char *name, uint32_t *ip);

// "no such name", "timed out", ...
const char *dns_error_text(int rcode);
const char *dns_type_name(uint16_t type);

#endif
