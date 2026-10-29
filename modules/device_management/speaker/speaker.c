/*
 * speaker.c - Drive the PC speaker through PIT channel 2.
 *
 * Uses: outb/inb (arch/x86/io), timer_wait and TIMER_HZ (timer/).
 */

#include "speaker.h"
#include "io.h"
#include "timer.h"
#include "def_speaker_freq_guard.h"   // DEFENCE speaker_freq_guard

#define PIT_CHANNEL2   0x42
#define PIT_COMMAND    0x43
#define PIT_CH2_SQUARE 0xB6     // channel 2, low+high byte, square wave
#define SPEAKER_PORT   0x61
#define SPEAKER_BITS   0x03     // bit 0: let PIT 2 run, bit 1: connect the speaker

void speaker_on(uint32_t hz) {
    // DEFENCE speaker_freq_guard: 0 divides by zero; the divisor must
    // fit 16 bits, and only 20 Hz..20 kHz is audible.
    if (!def_speaker_freq_ok(hz))
        return;

    uint32_t divisor = PIT_INPUT_HZ / hz;
    outb(PIT_COMMAND, PIT_CH2_SQUARE);
    outb(PIT_CHANNEL2, divisor & 0xFF);
    outb(PIT_CHANNEL2, (divisor >> 8) & 0xFF);
    outb(SPEAKER_PORT, inb(SPEAKER_PORT) | SPEAKER_BITS);
}

void speaker_off(void) {
    outb(SPEAKER_PORT, inb(SPEAKER_PORT) & ~SPEAKER_BITS);
}

void speaker_beep(uint32_t hz, uint32_t ms) {
    speaker_on(hz);
    timer_wait(ms * TIMER_HZ / 1000);
    speaker_off();
}
