#ifndef DEF_SPEAKER_FREQ_GUARD_H
#define DEF_SPEAKER_FREQ_GUARD_H

#include <stdint.h>

// Can the PC speaker play 'hz'? Below SPEAKER_MIN_HZ (20) the PIT
// divisor would not fit 16 bits - and 0 Hz divides by zero; above
// SPEAKER_MAX_HZ (20 kHz) nobody can hear it. speaker_on() in speaker.c
// asks this before it divides or writes the port. 1 = play, 0 = refused.
int def_speaker_freq_ok(uint32_t hz);

#endif
