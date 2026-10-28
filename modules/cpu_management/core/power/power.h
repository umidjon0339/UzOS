#ifndef POWER_H
#define POWER_H

/*
 * power.h - Power profiles: trade speed against energy.
 *
 * A laptop asks "fast or long battery life?". The OS answers with a few
 * switches. UzOS has two it can really turn:
 *
 *   profile        idle CPU does          one turn (slice)
 *   performance    POLL: spins (pause)    5 ticks = 50 ms
 *   balanced       C1: hlt until an IRQ   5 ticks = 50 ms     (default)
 *   power-saver    C1: hlt until an IRQ   10 ticks = 100 ms
 *
 * POLL wakes up a tiny bit faster but keeps the CPU busy, so it uses
 * full power while doing nothing (watch the host's CPU use in QEMU).
 * hlt stops the CPU until the next interrupt: the cheapest idle state
 * ("C1"). Longer turns mean fewer task switches, so the CPU and its
 * caches are disturbed less often.
 *
 * Linux has the same idea (power-profiles-daemon); there the profiles
 * also change clock speeds, which UzOS cannot (no cpufreq driver).
 */

#include <stdint.h>

typedef enum {
    POWER_PERFORMANCE,
    POWER_BALANCED,
    POWER_SAVER,
    POWER_PROFILE_COUNT
} power_profile_t;

power_profile_t power_profile(void);
void power_set_profile(power_profile_t profile);

// "performance", "balanced", "power-saver"; -1 / NULL if unknown.
const char *power_profile_name(power_profile_t profile);
int power_profile_find(const char *name);

// How long one turn on the CPU lasts in this profile, in timer ticks.
uint32_t power_slice_ticks(void);

// One step of the idle task: poll or hlt, depending on the profile.
// Called in a loop by idle_task (process_management/core/scheduler).
void power_idle(void);

// Idle state in use: "POLL" or "C1"; and how often the CPU entered
// each state since boot.
const char *power_idle_state(void);
uint32_t power_idle_entries(int halted);

#endif
