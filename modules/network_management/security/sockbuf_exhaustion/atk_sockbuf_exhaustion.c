/*
 * atk_sockbuf_exhaustion.c - ATTACK sockbuf_exhaustion: fill the packet pool through one sleeping socket
 *
 * Breaks:     resource - one socket cannot hold every packet buffer
 * Stopped by: rmem_limit (defence/rmem_limit)
 *
 * A datagram waits in its socket until the program reads it. Counting
 * only its bytes, 100 one-byte datagrams would cost 100 bytes, yet each
 * holds a whole packet buffer (about 1.7 KiB): the pool of 128 runs dry
 * and the whole stack stops.
 *
 * launch(): a UDP socket that never reads gets 100 one-byte datagrams.
 *           findings = datagrams kept beyond its rcvbuf.
 *
 * Uses: socket_* (core/socket), packet_free_count (core/packet), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/sockbuf_exhaustion/defence/rmem_limit/def_rmem_limit.h" // the charging rule
#include "modules/network_management/core/socket/socket.h"                      // socket_*
#include "modules/network_management/core/inet/inet.h"                          // INET_ADDR, NET_E*
#include "modules/network_management/core/packet/packet.h"                      // packet_free_count
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

#define PORT 7790

static int simulate(void) {
    ui_printf("  one byte of data still holds one whole packet buffer\n");
    ui_printf("  a socket that never reads could keep all %d of them\n", PACKET_POOL);
    return 1;
}

static int launch(void) {
    socket_t *sink, *src;
    if (socket_create(AF_INET, SOCK_DGRAM, 0, &sink) < 0)
        return 0;
    if (socket_create(AF_INET, SOCK_DGRAM, 0, &src) < 0) {
        socket_close(sink);
        return 0;
    }
    sockaddr_t a;
    uint32_t len = sizeof(a);
    socket_inet_addr(&a, &len, 0, PORT);
    socket_bind(sink, &a, len);
    len = sizeof(a);
    socket_inet_addr(&a, &len, INET_ADDR(127, 0, 0, 1), PORT);
    int lowest = packet_free_count();
    for (int i = 0; i < 100; i++) {
        socket_sendto(src, "x", 1, 0, &a, len);
        if (packet_free_count() < lowest)
            lowest = packet_free_count();
    }
    int kept = (int)sink->rx.count, limit = (int)(sink->rcvbuf / SOCKET_TRUESIZE);
    int extra = kept > limit ? kept - limit : 0;
    ui_printf("  100 datagrams of 1 byte: %d kept (rcvbuf: %d buffers), %u dropped\n", kept, limit, sink->drops);
    ui_printf("  free packet buffers never fell below %d of %d\n", lowest, PACKET_POOL);
    socket_close(src);
    socket_close(sink);
    ui_color(extra ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", extra ? "one socket holds the pool: the stack starves" : "the socket was cut off at its rcvbuf; the pool stays free");
    ui_reset();
    return extra;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "sockbuf_exhaustion",
    "tiny datagrams to a socket that never reads, until every packet buffer is its",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "rmem_limit"
};

void atk_sockbuf_exhaustion_register(void) { registry_add_attack(&attack); }
