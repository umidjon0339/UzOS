/*
 * atk_speaker_abuse.c - ATTACK speaker_abuse: a bad PC-speaker frequency.
 *
 * Breaks:     trust - a frequency is divided into; it must be checked
 * Stopped by: speaker_freq_guard (defence/speaker_freq_guard)
 *
 * speaker_on(hz) sets the PIT to 1193180 / hz and connects the speaker.
 * hz = 0 is a division by zero (a CPU exception); a very low hz makes a
 * divisor that does not fit the PIT's 16 bits; a huge hz is inaudible.
 * The guard lets only 20 Hz..20 kHz reach the divide and the port.
 *
 * simulate(): show what each bad frequency would do.
 * launch():   call the real speaker_on() with them; each is refused
 *             before any divide or port write, and the speaker stays off.
 * cleanup():  speaker_off(), in case anything is still sounding.
 *
 * Uses: speaker_on / speaker_off (speaker/), speaker_freq_guard, vga.
 */

#include "registry.h"
#include "audit.h"
#include "speaker.h"
#include "vga.h"

static const struct { uint32_t hz; const char *effect; } bad[] = {
    { 0,          "divisor 1193180/0: division by zero, CPU exception" },
    { 5,          "divisor 238636 > 65535: cut to 16 bits, wrong tone" },
    { 1000000,    "inaudible: 1 MHz is far above hearing" },
    { 0xFFFFFFFF, "absurd frequency" },
};
#define BADN (sizeof(bad) / sizeof(bad[0]))

static int simulate(void) {
    for (unsigned i = 0; i < BADN; i++) {
        vga_print_str("  speaker_on("); vga_print_int((int)bad[i].hz);
        vga_print_str("): "); vga_print_str(bad[i].effect); vga_print_str("\n");
    }
    return 1;
}

static int launch(void) {
    uint32_t before = audit_total();
    for (unsigned i = 0; i < BADN; i++)
        speaker_on(bad[i].hz);          // each refused before it divides
    speaker_off();                      // make sure nothing is sounding
    uint32_t refused = audit_total() - before;
    vga_print_str("  bad frequencies refused: "); vga_print_int((int)refused);
    vga_print_str(" of "); vga_print_int(BADN); vga_print_str("\n");
    return (int)(BADN - refused);       // 0 = none reached the divide
}

static int cleanup(void) { speaker_off(); return 0; }

static const attack_t attack = {
    "speaker_abuse",
    "a 0 or out-of-range speaker frequency: divide by zero or noise",
    INV_TRUST, SEV_LOW,
    simulate, launch, cleanup,
    "speaker_freq_guard"
};

void atk_speaker_abuse_register(void) { registry_add_attack(&attack); }
