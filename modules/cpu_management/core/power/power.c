/*
 * power.c - The current power profile and the idle loop it chooses.
 *
 * Uses: CPU_SLICE_TICKS (core/priority/priority.h), strcmp (lib/string).
 */

#include "modules/cpu_management/core/power/power.h"        // this part
#include "modules/cpu_management/core/priority/priority.h"  // CPU_SLICE_TICKS
#include "lib/string/string.h"                              // strcmp

static power_profile_t profile = POWER_BALANCED;
static uint32_t polls, halts;               // idle steps of each kind

static const char *names[POWER_PROFILE_COUNT] = {
    "performance", "balanced", "power-saver"
};

power_profile_t power_profile(void) {
    return profile;
}

void power_set_profile(power_profile_t p) {
    if (p < POWER_PROFILE_COUNT)
        profile = p;
}

const char *power_profile_name(power_profile_t p) {
    return p < POWER_PROFILE_COUNT ? names[p] : 0;
}

int power_profile_find(const char *name) {
    for (int p = 0; p < POWER_PROFILE_COUNT; p++)
        if (strcmp(names[p], name) == 0)
            return p;
    return -1;
}

uint32_t power_slice_ticks(void) {
    return profile == POWER_SAVER ? 2 * CPU_SLICE_TICKS : CPU_SLICE_TICKS;
}

void power_idle(void) {
    if (profile == POWER_PERFORMANCE) {
        polls++;
        asm volatile("pause");      // "I am spinning": a hint, the CPU stays awake
    } else {
        halts++;
        asm volatile("hlt");        // sleep until the next interrupt
    }
}

const char *power_idle_state(void) {
    return profile == POWER_PERFORMANCE ? "POLL" : "C1";
}

uint32_t power_idle_entries(int halted) {
    return halted ? halts : polls;
}
