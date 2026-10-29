# speaker_abuse -> speaker_freq_guard

**Attack.** `speaker_on(hz)` programs the PIT with `1193180 / hz`. A 0
divides by zero (CPU exception); a very low hz overflows the 16-bit
divisor; a huge hz is inaudible noise.

**Defence (speaker_freq_guard).** `speaker_on()` asks `def_speaker_freq_ok`
first and only lets 20 Hz..20 kHz reach the divide and the speaker port.
Everything else is refused and logged.
