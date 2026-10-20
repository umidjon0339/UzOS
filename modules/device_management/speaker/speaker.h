#ifndef SPEAKER_H
#define SPEAKER_H

/*
 * speaker.h - The PC speaker: one square-wave tone at a time.
 *
 * PIT channel 2 makes a square wave at 1193180 / divisor Hz. Port 0x61
 * connects that wave to the little speaker (bits 0 and 1 on).
 */

#include <stdint.h>

#define SPEAKER_MIN_HZ 20       // below this the divisor would not fit 16 bits
#define SPEAKER_MAX_HZ 20000    // above this nobody hears it anyway

// Start a tone (out-of-range frequencies are ignored).
void speaker_on(uint32_t hz);

// Silence.
void speaker_off(void);

// Play 'hz' for 'ms' milliseconds, then stop. Waits while it plays.
void speaker_beep(uint32_t hz, uint32_t ms);

#endif
