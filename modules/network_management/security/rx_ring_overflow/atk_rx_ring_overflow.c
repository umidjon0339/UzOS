/*
 * atk_rx_ring_overflow.c - ATTACK rx_ring_overflow (ILLUSTRATION): a descriptor longer than its buffer
 *
 * Breaks:     trust - the driver never reads past one receive buffer
 * Stopped by: rx_desc_check (defence/rx_desc_check) - ILLUSTRATION
 *
 * The card writes each frame's length into its descriptor. A buggy or
 * malicious device (or a forged descriptor) could say 4000 for a
 * 2048-byte buffer; a driver that believes it copies 2 KiB of the next
 * buffer, or past the ring. QEMU's card is honest, so this is an
 * ILLUSTRATION: forged descriptor values go through the driver's own check.
 *
 * simulate(): explain the descriptor fields.
 * launch():   check three forged descriptors. findings = accepted ones.
 * cleanup():  nothing.
 *
 * Uses: def_rx_desc_ok (its defence), ui_*.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/rx_ring_overflow/defence/rx_desc_check/def_rx_desc_check.h" // def_rx_desc_ok
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static int simulate(void) {
    ui_printf("  an RX descriptor: buffer address, LENGTH, status (DD, EOP), errors\n");
    ui_printf("  the driver copies 'length' bytes out of a 2048-byte buffer\n");
    ui_printf("  a lying length makes it read the next buffer, or past the ring\n");
    return 1;
}

static int launch(void) {
    static const struct { uint8_t status, errors; uint16_t len; const char *what; } forged[3] = {
        { 0x3, 0, 4000, "length 4000 > 2048" },
        { 0x1, 0, 600,  "no EOP: a frame split over buffers" },
        { 0x3, 0x80, 60, "the card flagged an error" },
    };
    int accepted = 0;
    ui_printf("  (QEMU's card never lies: these descriptors are forged by hand)\n");
    for (int i = 0; i < 3; i++) {
        int ok = def_rx_desc_ok(forged[i].status, forged[i].errors, forged[i].len, 2048);
        ui_printf("  %-38s -> %s\n", forged[i].what, ok ? "ACCEPTED" : "refused");
        accepted += ok;
    }
    return accepted;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "rx_ring_overflow",
    "a receive descriptor claims more bytes than its buffer holds",
    INV_TRUST, SEV_MEDIUM,
    simulate, launch, cleanup,
    "rx_desc_check"
};

void atk_rx_ring_overflow_register(void) { registry_add_attack(&attack); }
