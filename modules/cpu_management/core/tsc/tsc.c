/*
 * tsc.c - Read the cycle counter and measure the clock speed.
 *
 * Uses: cpu_rdtsc (arch/x86/cpu), cpu_has (core/info), timer_wait /
 *       timer_frequency (device_management/timer), and the defence
 *       tsc_restrict to keep the counter away from ring 3.
 */

#include "tsc.h"
#include "cpu.h"
#include "cpu_info.h"
#include "timer.h"
#include "def_tsc_restrict.h"

static uint32_t mhz;

int tsc_available(void) {
    return cpu_has(CPU_FEAT_TSC);
}

uint64_t tsc_now(void) {
    return tsc_available() ? cpu_rdtsc() : 0;
}

void tsc_init(void) {
    if (!tsc_available() || !timer_frequency())
        return;

    // Start right after a tick, so we measure whole ticks.
    timer_wait(1);
    uint64_t start = cpu_rdtsc();
    timer_wait(TSC_CALIBRATE_TICKS);
    uint64_t cycles = cpu_rdtsc() - start;

    // cycles per second = cycles / (ticks / ticks-per-second)
    uint64_t per_second = cycles * timer_frequency() / TSC_CALIBRATE_TICKS;
    mhz = (uint32_t)(per_second / 1000000);

    // DEFENCE tsc_restrict: from now on rdtsc works in ring 0 only.
    def_tsc_restrict_enable();
}

uint32_t tsc_mhz(void) {
    return mhz;
}

uint32_t tsc_cycles_to_us(uint64_t cycles) {
    return mhz ? (uint32_t)(cycles / mhz) : 0;
}

uint32_t tsc_cycles_to_ns(uint64_t cycles) {
    return mhz ? (uint32_t)(cycles * 1000 / mhz) : 0;
}
