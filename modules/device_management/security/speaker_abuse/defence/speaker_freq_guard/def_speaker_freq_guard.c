/*
 * def_speaker_freq_guard.c - DEFENCE speaker_freq_guard: only audible,
 *                            dividable PC-speaker tones.
 *
 * Stops:  speaker_abuse (security/speaker_abuse)
 * Called: speaker_on() (speaker/speaker.c), before dividing or writing.
 * Rule:   SPEAKER_MIN_HZ <= hz <= SPEAKER_MAX_HZ.
 * Why:    speaker_on() computes divisor = 1193180 / hz and writes it to
 *         the PIT. hz = 0 divides by zero; hz below 20 gives a divisor
 *         that does not fit 16 bits; hz above 20 kHz is inaudible noise.
 */

#include "def_speaker_freq_guard.h"
#include "registry.h"
#include "audit.h"
#include "speaker.h"            // SPEAKER_MIN_HZ, SPEAKER_MAX_HZ

static int freq_ok(uint32_t hz) {
    return hz >= SPEAKER_MIN_HZ && hz <= SPEAKER_MAX_HZ;
}

int def_speaker_freq_ok(uint32_t hz) {
    if (freq_ok(hz))
        return 1;
    audit_log("speaker_freq_guard", "speaker frequency refused: 0, too low or too high");
    return 0;
}

static int selftest(void) {
    if (freq_ok(0)) return 0;                     // divide by zero
    if (freq_ok(SPEAKER_MIN_HZ - 1)) return 0;    // divisor too big
    if (!freq_ok(SPEAKER_MIN_HZ)) return 0;       // lowest audible
    if (!freq_ok(1000)) return 0;                 // a normal beep
    if (!freq_ok(SPEAKER_MAX_HZ)) return 0;       // highest audible
    if (freq_ok(SPEAKER_MAX_HZ + 1)) return 0;    // inaudible
    if (freq_ok(0xFFFFFFFF)) return 0;            // absurd
    return 1;
}

static const defence_t defence = {
    "speaker_freq_guard",
    "only play PC-speaker tones from 20 Hz to 20 kHz (never divide by 0)",
    "speaker_abuse",
    selftest
};

void def_speaker_freq_guard_register(void) { registry_add_defence(&defence); }
