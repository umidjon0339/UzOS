/*
 * atk_malformed_frame.c - ATTACK malformed_frame: feed the receive path broken Ethernet frames
 *
 * Breaks:     trust - every frame the stack reads is a well-formed Ethernet frame
 * Stopped by: frame_validate (defence/frame_validate)
 *
 * Parsers that trust lengths read past the end of a buffer. A frame
 * shorter than its header, one longer than Ethernet allows, or an
 * 802.3 frame whose "type" field is really a length must never reach
 * the protocol handlers.
 *
 * simulate(): list the four broken frames.
 * launch():   inject them on eth0. findings = frames that got past the
 *             check (counted as receive errors when refused).
 * cleanup():  nothing to undo.
 *
 * Uses: netdev_inject (core/netdev), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/malformed_frame/defence/frame_validate/def_frame_validate.h" // FRAME_MAX
#include "modules/network_management/core/netdev/netdev.h"                      // netdev_by_name, netdev_inject, netdev_lock
#include "modules/network_management/core/ethernet/ethernet.h"                  // eth_header_t, ETH_*
#include "modules/network_management/core/arp/arp.h"                            // arp_packet_t, arp_lookup, arp_entry
#include "modules/network_management/core/inet/inet.h"                          // htons, htonl, inet_format, mac_format
#include "lib/string/string.h"                                                  // memcpy, memset
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static uint8_t frame[1560];

// A frame to us with 'type' and 'len' bytes in all (rest zero).
static int build(const netdev_t *eth, uint16_t type, int len) {
    memset(frame, 0, sizeof(frame));
    eth_header_t *h = (eth_header_t *)frame;
    memcpy(h->dest, eth->mac, 6);
    memcpy(h->src, eth->mac, 6);
    h->src[0] = 0x02;
    h->type = htons(type);
    return len;
}

static int simulate(void) {
    ui_printf("  runt: 10 bytes (shorter than the 14-byte header)\n");
    ui_printf("  13 bytes: one byte short of a header\n");
    ui_printf("  giant: 1530 bytes (Ethernet carries at most 1514)\n");
    ui_printf("  802.3: type 0x0040 is a length, not a protocol\n");
    return 4;
}

static int launch(void) {
    netdev_t *eth = netdev_by_name("eth0");
    if (!eth)
        return 0;
    static const struct { uint16_t type; int len; const char *what; } bad[4] = {
        { ETH_P_IP, 10, "runt (10 bytes)" }, { ETH_P_IP, 13, "13 bytes" },
        { ETH_P_IP, 1530, "giant (1530 bytes)" }, { 0x0040, 60, "802.3 length field" },
    };
    int got_past = 0;
    for (int i = 0; i < 4; i++) {
        uint32_t errors = eth->stats.rx_errors;
        netdev_inject(eth, frame, (uint16_t)build(eth, bad[i].type, bad[i].len));
        int refused = eth->stats.rx_errors > errors;
        ui_printf("  %-22s -> %s\n", bad[i].what, refused ? "refused" : "PASSED");
        got_past += !refused;
    }
    ui_color(got_past ? UI_BAD : UI_GOOD);
    ui_printf("  %s\n", got_past ? "broken frames reached the protocols" : "every broken frame was dropped at the door");
    ui_reset();
    return got_past;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "malformed_frame",
    "send runt, giant and old-style (802.3) frames to confuse the receive path",
    INV_TRUST, SEV_LOW,
    simulate, launch, cleanup,
    "frame_validate"
};

void atk_malformed_frame_register(void) { registry_add_attack(&attack); }
