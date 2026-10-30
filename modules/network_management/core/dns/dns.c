/*
 * dns.c - The resolver: hosts file, cache, and questions to the server.
 *
 * Reading an answer is where resolvers get attacked, so every step is
 * checked against the length of the datagram: a name may not run past
 * the end, and a compression pointer may only be followed a few times
 * (a pointer to itself would otherwise loop forever).
 *
 * Uses: socket_* (core/socket), dhcp_lease (core/dhcp), inet_* (core/inet),
 *       net_now_ms (core/net_tick), tsc_now (cpu_management), vfs_lookup /
 *       vfs_read / vfs_write (filesystem_management), ramdisk_create /
 *       ramdisk_truncate (filesystem_management), format, strcpy, memcpy.
 */

#include "modules/network_management/core/dns/dns.h"             // this file
#include "modules/network_management/core/socket/socket.h"       // socket_create, socket_sendto, ...
#include "modules/network_management/core/dhcp/dhcp.h"          // dhcp_lease
#include "modules/network_management/core/inet/inet.h"          // inet_parse, inet_format, htons
#include "modules/network_management/core/net_tick/net_tick.h"  // net_now_ms
#include "modules/cpu_management/core/tsc/tsc.h"                // tsc_now
#include "modules/filesystem_management/core/vfs/vfs.h"         // vfs_lookup, vfs_read, vfs_write
#include "modules/filesystem_management/core/ramdisk/ramdisk.h" // ramdisk_create, ramdisk_truncate
#include "lib/format/format.h"                                  // format
#include "lib/string/string.h"                                  // strcpy, strlen, memcpy, memset
#include "modules/network_management/security/dns_spoofing/defence/dns_txid_port_random/def_dns_txid_port_random.h"  // def_dns_id, def_dns_port

#define MSG_MAX        512          // classic DNS over UDP
#define FLAG_QR        0x8000       // this is a response
#define FLAG_RD        0x0100       // recursion desired: "find it for me"
#define CLASS_IN       1
#define MAX_POINTERS   8            // compression pointers followed per name
#define CACHE_SIZE     16
#define CACHE_MAX_TTL  300          // seconds: never trust an answer longer
#define FILE_MAX       2048         // bytes read of /etc/hosts and resolv.conf

typedef struct {
    uint16_t id, flags, qdcount, ancount, nscount, arcount;   // network byte order
} __attribute__((packed)) dns_header_t;

typedef struct {
    char name[DNS_NAME_MAX];
    uint32_t ip;
    uint32_t expires_ms;
} cache_entry_t;

static cache_entry_t cache[CACHE_SIZE];

/* ---------- Small text helpers ---------- */

static char lower(char c) {
    return c >= 'A' && c <= 'Z' ? (char)(c + 32) : c;
}

// Names compare without case: Example.COM == example.com
static int same_name(const char *a, const char *b) {
    while (*a && lower(*a) == lower(*b))
        a++, b++;
    return lower(*a) == lower(*b);
}

/* ---------- Files ---------- */

// Read a whole small file into buf (NUL-terminated). 0 if missing.
static int read_file(const char *path, char *buf, uint32_t max) {
    vfs_node_t *f = vfs_lookup(fs_root, path);
    if (!f || !(f->flags & FS_FILE))
        return 0;
    uint32_t n = vfs_read(f, 0, max - 1, (uint8_t *)buf);
    buf[n] = '\0';
    return 1;
}

static void write_file(const char *path, const char *text) {
    vfs_node_t *f = vfs_lookup(fs_root, path);
    if (!f) {
        vfs_node_t *etc = vfs_lookup(fs_root, "/etc");
        if (!etc)
            etc = ramdisk_create(fs_root, "etc", FS_DIRECTORY);
        f = etc ? ramdisk_create(etc, path + 5, FS_FILE) : 0;   // path + 5: after "/etc/"
    }
    if (!f)
        return;
    ramdisk_truncate(f);
    vfs_write(f, 0, strlen(text), (uint8_t *)text);
}

// The next whitespace-separated word of a line. Returns where it ends.
static const char *word(const char *s, char *out, int max) {
    while (*s == ' ' || *s == '\t')
        s++;
    int n = 0;
    while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '#') {
        if (n < max - 1)
            out[n++] = *s;
        s++;
    }
    out[n] = '\0';
    return s;
}

static const char *next_line(const char *s) {
    while (*s && *s != '\n')
        s++;
    return *s ? s + 1 : s;
}

void dns_init(void) {
    if (!vfs_lookup(fs_root, "/etc/hosts"))
        write_file("/etc/hosts", "# name -> address, read before asking DNS\n"
                                 "127.0.0.1   localhost\n"
                                 "127.0.1.1   uzos\n");
    if (!vfs_lookup(fs_root, "/etc/resolv.conf"))
        write_file("/etc/resolv.conf", "# the DNS server (DHCP writes it)\n");
}

// /etc/hosts: "address name name ..." per line.
static int hosts_lookup(const char *name, uint32_t *ip) {
    static char text[FILE_MAX];
    if (!read_file("/etc/hosts", text, sizeof(text)))
        return 0;
    for (const char *line = text; *line; line = next_line(line)) {
        char addr[20], alias[DNS_NAME_MAX];
        const char *rest = word(line, addr, sizeof(addr));
        uint32_t candidate;
        if (!inet_parse(addr, &candidate))
            continue;
        for (rest = word(rest, alias, sizeof(alias)); alias[0]; rest = word(rest, alias, sizeof(alias))) {
            if (same_name(alias, name)) {
                *ip = candidate;
                return 1;
            }
        }
    }
    return 0;
}

uint32_t dns_nameserver(void) {
    static char text[FILE_MAX];
    if (read_file("/etc/resolv.conf", text, sizeof(text))) {
        for (const char *line = text; *line; line = next_line(line)) {
            char key[16], value[20];
            const char *rest = word(line, key, sizeof(key));
            word(rest, value, sizeof(value));
            uint32_t ip;
            if (strcmp(key, "nameserver") == 0 && inet_parse(value, &ip))
                return ip;
        }
    }
    const dhcp_lease_t *lease = dhcp_lease();
    return lease->state == DHCP_BOUND ? lease->dns : 0;
}

void dns_set_nameserver(uint32_t ip) {
    char text[80], addr[INET_ADDR_TEXT];
    inet_format(ip, addr);
    format(text, sizeof(text), "# the DNS server (DHCP writes it)\nnameserver %s\n", addr);
    write_file("/etc/resolv.conf", text);
}

/* ---------- Building a question ---------- */

// "www.example.com" -> 3www7example3com0. Returns bytes written, or 0.
static int encode_name(const char *name, uint8_t *out, int max) {
    int pos = 0;
    while (*name) {
        int len = 0;
        while (name[len] && name[len] != '.')
            len++;
        if (len == 0 || len > 63 || pos + len + 2 > max)
            return 0;                       // empty or too long label
        out[pos++] = (uint8_t)len;
        memcpy(out + pos, name, len);
        pos += len;
        name += len;
        if (*name == '.')
            name++;
    }
    out[pos++] = 0;                         // the root: end of the name
    return pos;
}

static int build_query(uint8_t *msg, uint16_t id, const char *name, uint16_t type) {
    dns_header_t *h = (dns_header_t *)msg;
    memset(h, 0, sizeof(*h));
    h->id = htons(id);
    h->flags = htons(FLAG_RD);
    h->qdcount = htons(1);
    int n = encode_name(name, msg + sizeof(*h), MSG_MAX - sizeof(*h) - 4);
    if (!n)
        return 0;
    uint8_t *q = msg + sizeof(*h) + n;
    q[0] = (uint8_t)(type >> 8);
    q[1] = (uint8_t)type;
    q[2] = 0;
    q[3] = CLASS_IN;
    return (int)sizeof(*h) + n + 4;
}

/* ---------- Reading an answer ---------- */

static uint16_t get16(const uint8_t *p) { return (uint16_t)((p[0] << 8) | p[1]); }
static uint32_t get32(const uint8_t *p) { return ((uint32_t)get16(p) << 16) | get16(p + 2); }

// Read the name at *pos into out ("www.example.com"), following
// compression pointers. *pos moves past the name as it is stored here.
// Returns 0 if it runs out of the message or loops.
static int read_name(const uint8_t *msg, int len, int *pos, char *out, int max) {
    int at = *pos, n = 0, jumped = 0, pointers = 0;
    for (;;) {
        if (at >= len)
            return 0;
        uint8_t size = msg[at];
        if ((size & 0xC0) == 0xC0) {        // a pointer: the rest of the name is elsewhere
            if (at + 1 >= len || ++pointers > MAX_POINTERS)
                return 0;
            if (!jumped)
                *pos = at + 2;
            jumped = 1;
            at = ((size & 0x3F) << 8) | msg[at + 1];
            continue;
        }
        if (size == 0)
            break;
        if (size > 63 || at + 1 + size > len || n + size + 2 > max)
            return 0;
        if (n)
            out[n++] = '.';
        memcpy(out + n, msg + at + 1, size);
        n += size;
        at += 1 + size;
    }
    out[n] = '\0';
    if (!jumped)
        *pos = at + 1;
    return 1;
}

static void read_record_data(const uint8_t *msg, int len, int at, uint16_t rdlen, dns_record_t *r) {
    int pos = at;
    r->text[0] = '\0';
    switch (r->type) {
    case DNS_TYPE_A:
        if (rdlen == 4)
            r->ip = get32(msg + at);
        break;
    case DNS_TYPE_CNAME:
    case DNS_TYPE_NS:
    case DNS_TYPE_PTR:
        read_name(msg, len, &pos, r->text, sizeof(r->text));
        break;
    case DNS_TYPE_MX:
        if (rdlen >= 3) {
            r->preference = get16(msg + at);
            pos = at + 2;
            read_name(msg, len, &pos, r->text, sizeof(r->text));
        }
        break;
    case DNS_TYPE_TXT: {
        int size = rdlen ? msg[at] : 0;     // the first character-string
        if (size >= (int)sizeof(r->text))
            size = sizeof(r->text) - 1;
        if (size > rdlen - 1)
            size = rdlen - 1;
        memcpy(r->text, msg + at + 1, size > 0 ? size : 0);
        r->text[size > 0 ? size : 0] = '\0';
        break;
    }
    case DNS_TYPE_AAAA:
        if (rdlen == 16) {
            int n = 0;
            for (int i = 0; i < 16; i += 2)
                n += format(r->text + n, sizeof(r->text) - n, i ? ":%x" : "%x", get16(msg + at + i));
        }
        break;
    }
}

// Is 'msg' the answer to our question? Fill 'out'. 1 if it is ours.
static int parse_answer(const uint8_t *msg, int len, uint16_t id, const char *name,
                        uint16_t type, dns_result_t *out) {
    if (len < (int)sizeof(dns_header_t))
        return 0;
    const dns_header_t *h = (const dns_header_t *)msg;
    uint16_t flags = ntohs(h->flags);
    if (ntohs(h->id) != id || !(flags & FLAG_QR) || ntohs(h->qdcount) != 1)
        return 0;                           // not the answer to our question

    // The question must be ours too.
    int pos = sizeof(dns_header_t);
    char qname[DNS_NAME_MAX];
    if (!read_name(msg, len, &pos, qname, sizeof(qname)) || pos + 4 > len ||
        !same_name(qname, name) || get16(msg + pos) != type)
        return 0;
    pos += 4;

    out->flags = flags;
    out->rcode = flags & 0xF;
    out->count = 0;
    int answers = ntohs(h->ancount);
    for (int i = 0; i < answers && out->count < DNS_MAX_RECORDS; i++) {
        dns_record_t *r = &out->answers[out->count];
        if (!read_name(msg, len, &pos, r->name, sizeof(r->name)) || pos + 10 > len)
            break;
        r->type = get16(msg + pos);
        uint16_t class = get16(msg + pos + 2);
        r->ttl = get32(msg + pos + 4);
        uint16_t rdlen = get16(msg + pos + 8);
        pos += 10;
        if (pos + rdlen > len)
            break;                          // the data runs past the end
        if (class == CLASS_IN) {
            read_record_data(msg, len, pos, rdlen, r);
            out->count++;
        }
        pos += rdlen;
    }
    return 1;
}

/* ---------- Asking ---------- */

int dns_query(const char *name, uint16_t type, uint32_t server, dns_result_t *out) {
    static uint8_t msg[MSG_MAX];
    memset(out, 0, sizeof(*out));
    if (!server)
        server = dns_nameserver();
    out->server = server;
    if (!server)
        return out->rcode = DNS_ERR_NOSERVER;

    uint16_t id = def_dns_id();                 // DEFENCE dns_txid_port_random: 16 random bits ...
    out->id = id;
    int len = build_query(msg, id, name, type);
    if (!len)
        return out->rcode = DNS_ERR_BADNAME;

    socket_t *s;
    if (socket_create(AF_INET, SOCK_DGRAM, 0, &s) < 0)
        return out->rcode = DNS_ERR_NETWORK;
    socket_timeval_t timeout = { 0, DNS_TIMEOUT_MS * 1000 };
    socket_setopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    sockaddr_t to;
    uint32_t to_len = sizeof(to);
    socket_inet_addr(&to, &to_len, server, DNS_PORT);
    // ... and 16 more in a random source port.
    for (int tries = 0; tries < 8; tries++) {
        sockaddr_t me;
        uint32_t me_len = sizeof(me);
        socket_inet_addr(&me, &me_len, 0, def_dns_port());
        if (socket_bind(s, &me, me_len) == 0)
            break;                              // (taken: draw another)
    }
    // connect: only datagrams from the server's port 53 get to us.
    if (socket_connect(s, &to, to_len) < 0) {
        socket_close(s);
        return out->rcode = DNS_ERR_NETWORK;
    }

    uint32_t start = net_now_ms();
    out->rcode = DNS_ERR_TIMEOUT;
    for (int attempt = 0; attempt < DNS_TRIES && out->rcode == DNS_ERR_TIMEOUT; attempt++) {
        if (socket_sendto(s, msg, (uint32_t)len, 0, 0, 0) < 0) {
            out->rcode = DNS_ERR_NETWORK;
            break;
        }
        static uint8_t reply[MSG_MAX];
        int n;
        while ((n = socket_recvfrom(s, reply, sizeof(reply), 0, 0, 0)) > 0)
            if (parse_answer(reply, n, id, name, type, out))
                break;                      // ours (anything else is ignored)
        if (n == -NET_EINTR)
            break;                          // Ctrl+C
    }
    out->time_ms = net_now_ms() - start;
    socket_close(s);
    if (out->rcode == DNS_OK && out->count == 0)
        out->rcode = DNS_ERR_NODATA;
    return out->rcode;
}

/* ---------- The normal way: dns_resolve ---------- */

static int cache_lookup(const char *name, uint32_t *ip) {
    uint32_t now = net_now_ms();
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].name[0] && same_name(cache[i].name, name) &&
            (int32_t)(cache[i].expires_ms - now) > 0) {
            *ip = cache[i].ip;
            return 1;
        }
    }
    return 0;
}

static void cache_add(const char *name, uint32_t ip, uint32_t ttl) {
    uint32_t now = net_now_ms();
    int slot = 0;
    for (int i = 0; i < CACHE_SIZE; i++) {          // a free (or expired) slot, else the oldest
        if ((int32_t)(cache[i].expires_ms - now) <= 0) {
            slot = i;
            break;
        }
        if (cache[i].expires_ms < cache[slot].expires_ms)
            slot = i;
    }
    if (strlen(name) >= DNS_NAME_MAX)
        return;
    strcpy(cache[slot].name, name);
    cache[slot].ip = ip;
    cache[slot].expires_ms = now + (ttl > CACHE_MAX_TTL ? CACHE_MAX_TTL : ttl) * 1000;
}

int dns_resolve(const char *name, uint32_t *ip) {
    if (inet_parse(name, ip))
        return DNS_OK;                      // already an address
    if (hosts_lookup(name, ip) || cache_lookup(name, ip))
        return DNS_OK;

    static dns_result_t result;
    int rcode = dns_query(name, DNS_TYPE_A, 0, &result);
    if (rcode != DNS_OK)
        return rcode;
    // The answer may be a chain: www.x.com CNAME x.com, x.com A 1.2.3.4.
    for (int i = 0; i < result.count; i++) {
        if (result.answers[i].type == DNS_TYPE_A) {
            *ip = result.answers[i].ip;
            cache_add(name, *ip, result.answers[i].ttl);
            return DNS_OK;
        }
    }
    return DNS_ERR_NODATA;
}

/* ---------- Names for messages ---------- */

const char *dns_error_text(int rcode) {
    switch (rcode) {
    case DNS_OK:           return "ok";
    case DNS_FORMERR:      return "format error";
    case DNS_SERVFAIL:     return "server failure";
    case DNS_NXDOMAIN:     return "not found: 3(NXDOMAIN)";
    case DNS_REFUSED:      return "refused";
    case DNS_ERR_TIMEOUT:  return "connection timed out; no servers could be reached";
    case DNS_ERR_NOSERVER: return "no DNS server (dhclient, or nameserver in /etc/resolv.conf)";
    case DNS_ERR_BADNAME:  return "not a valid name";
    case DNS_ERR_NETWORK:  return "network unreachable";
    case DNS_ERR_NODATA:   return "has no record of that type";
    default:               return "error";
    }
}

const char *dns_type_name(uint16_t type) {
    switch (type) {
    case DNS_TYPE_A:     return "A";
    case DNS_TYPE_NS:    return "NS";
    case DNS_TYPE_CNAME: return "CNAME";
    case DNS_TYPE_PTR:   return "PTR";
    case DNS_TYPE_MX:    return "MX";
    case DNS_TYPE_TXT:   return "TXT";
    case DNS_TYPE_AAAA:  return "AAAA";
    default:             return "?";
    }
}
