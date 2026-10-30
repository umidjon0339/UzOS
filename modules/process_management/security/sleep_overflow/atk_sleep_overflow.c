/*
 * atk_sleep_overflow.c - ATTACK sleep_overflow: a time that wraps around.
 *
 * Breaks:     liveness - a wait must last as long as it was asked to
 * Stopped by: sleep_bounds (defence/sleep_bounds)
 *
 * The kernel turns a program's time into timer ticks. Done in 32 bits,
 * big numbers wrap: 42949673 seconds * 100 ticks becomes 30 ticks
 * (0.3 s), and (0xFFFFFFFF ms + 9) / 10 becomes 0. A protective timeout
 * then fires at once; a deadline in the "far future" is already past.
 * Malformed times (nsec >= 1e9, negative) give nonsense too.
 *
 * simulate(): show what 32-bit arithmetic makes of the evil inputs.
 * launch():   feed them to sleep_bounds, exactly as nanosleep, setitimer
 *             and sleep() do. findings = results that wrapped or were
 *             accepted when they should be refused.
 * cleanup():  nothing.
 *
 * Uses: def_sleep_ms_ticks / def_sleep_time_ticks (its defence), ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/security/sleep_overflow/defence/sleep_bounds/def_sleep_bounds.h"  // def_sleep_ms_ticks, def_sleep_time_ticks
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

static int simulate(void) {
    uint32_t sec = 42949673, ms = 0xFFFFFFFFu;
    ui_printf("  nanosleep(%u s):  32-bit sec*100 = %u ticks (%u.%02u s)\n",
              sec, sec * 100u, sec * 100u / 100, sec * 100u % 100);
    ui_printf("  sleep(0xFFFFFFFF ms): (ms + 9) / 10 = %u ticks\n", (ms + 9) / 10);
    ui_printf("  nanosleep(0 s, 1000000000 ns): not a valid time at all\n");
    return 3;
}

static void verdict(const char *what, int bad, const char *result) {
    ui_printf("  %-30s ", what);
    ui_color(bad ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", result);
    ui_reset();
}

static int launch(void) {
    int bad = 0;
    uint32_t t = 0;

    int ok = def_sleep_time_ticks(42949673, 0, 1000000000, &t);
    int wrapped = !ok || t < 0x10000000u;
    bad += wrapped;
    verdict("nanosleep(42949673 s)", wrapped, wrapped ? "wrapped to a short sleep!" : "capped at ~248 days");

    t = def_sleep_ms_ticks(0xFFFFFFFFu);
    bad += t < 0x10000000u;
    verdict("sleep(0xFFFFFFFF ms)", t < 0x10000000u, t < 0x10000000u ? "wrapped to 0!" : "~49 days, as asked");

    ok = def_sleep_time_ticks(0, 1000000000, 1000000000, &t);
    bad += ok;
    verdict("nanosleep(0 s, 1e9 ns)", ok, ok ? "accepted!" : "refused (EINVAL)");

    ok = def_sleep_time_ticks(-5, 0, 1000000, &t);
    bad += ok;
    verdict("setitimer(-5 s)", ok, ok ? "accepted!" : "refused (EINVAL)");
    return bad;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "sleep_overflow",
    "huge or malformed times wrap the kernel's tick arithmetic",
    INV_LIVENESS, SEV_LOW,
    simulate, launch, cleanup,
    "sleep_bounds"
};

void atk_sleep_overflow_register(void) { registry_add_attack(&attack); }
