# cache_timing -> coarse_timer  (illustration)

**Attack (illustration).** A cached byte loads far faster than one from
RAM. Timing accesses reveals which addresses a victim touched, leaking
secrets. It needs a fine clock.

**Defence (coarse_timer).** UzOS traps ring 3 `rdtsc` (CR4.TSD, shared
with side_channel), so a ring 3 attacker's finest clock is the 10 ms
timer tick - millions of cycles, far too coarse to tell a cache hit
(~50 cycles) from a miss (~250). No clock, no channel.
