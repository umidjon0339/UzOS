/*
 * atk_timer_divide_by_zero.c - ATTACK timer_divide_by_zero: a bad PIT rate.
 *
 * Breaks:     trust - a number must be checked before dividing by it
 * Stopped by: timer_frequency (security/defence/timer_frequency)
 *
 * init_timer(hz) computes 1193180 / hz. hz = 0 is a division by zero,
 * and other values give a divisor that does not fit the 16-bit counter.
 *
 * simulate(): show what each bad rate would do.
 * launch():   call the real init_timer() with them; the tick rate must
 *             stay at TIMER_HZ.
 *
 * Uses: init_timer / timer_frequency (timer/).
 */

#include "security_lab.h"
#include "timer.h"
#include "vga.h"

static const struct {
    uint32_t hz;
    const char *effect;
} bad_rates[] = {
    { 0,       "division by zero: CPU exception, kernel halts" },
    { 10,      "divisor 119318 > 65535: cut to 16 bits, runs at 22 Hz" },
    { 2000000, "divisor 0: the PIT counts 65536, runs at 18 Hz" },
};
#define BAD_COUNT (sizeof(bad_rates) / sizeof(bad_rates[0]))

static int simulate(void) {
    for (unsigned i = 0; i < BAD_COUNT; i++) {
        vga_print_str("  init_timer(");
        vga_print_int(bad_rates[i].hz);
        vga_print_str("): ");
        vga_print_str(bad_rates[i].effect);
        vga_print_str("\n");
    }
    return BAD_COUNT;
}

static int launch(void) {
    uint32_t denied = sec_total_denied();

    for (unsigned i = 0; i < BAD_COUNT; i++)
        init_timer(bad_rates[i].hz);

    vga_print_str("  tick rate after the attack: ");
    vga_print_int(timer_frequency());
    vga_print_str(timer_frequency() == TIMER_HZ ? " Hz (unchanged)\n" : " Hz (CHANGED)\n");
    return sec_total_denied() - denied;
}

static const attack_t attack = {
    "timer_divide_by_zero",
    "start the timer at 0 Hz (divide by zero) or a rate the PIT cannot do",
    INV_TRUST, SEV_CRITICAL,
    simulate, launch, 0,        // every call was refused: nothing changed
    "timer_frequency"
};

void atk_timer_divide_by_zero_register(void) {
    proc_security_register_attack(&attack);
}
