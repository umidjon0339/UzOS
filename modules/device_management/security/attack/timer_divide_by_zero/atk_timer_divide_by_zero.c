#include <stdint.h>
#include "device_security.h"
#include "proc_security.h"
#include "timer.h"
#include "vga.h"

static int atk_timer_divide_by_zero_check(void) {
    static const uint32_t probes[] = {
        0, PIT_MIN_FREQUENCY - 1, PIT_MIN_FREQUENCY,
        PIT_INPUT_HZ, PIT_INPUT_HZ + 1, 0xFFFFFFFFu
    };
    static const int expected[] = { 0, 0, 1, 1, 0, 0 };
    int blocked = 0;

    for (unsigned int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
        if (device_timer_frequency_valid(probes[i]) == expected[i]) blocked++;
    return blocked;
}

static int atk_timer_divide_by_zero_simulate(void) {
    vga_print_str("  probes zero and frequencies outside the 16-bit PIT divisor range\n");
    vga_print_str("  no ports are written\n");
    return atk_timer_divide_by_zero_check();
}

static int atk_timer_divide_by_zero_launch(void) {
    uint32_t denied_before = sec_total_denied();

    vga_print_str("  submitting frequency 0; guard must refuse before PIT I/O\n");
    init_timer(0);
    return atk_timer_divide_by_zero_check() +
           (sec_total_denied() > denied_before);
}

static const attack_t atk = {
    "timer_divide_by_zero",
    "a zero timer frequency causes division by zero during PIT setup",
    INV_RESOURCE,
    SEV_CRITICAL,
    atk_timer_divide_by_zero_simulate,
    atk_timer_divide_by_zero_launch,
    0,
    "timer_frequency"
};

void atk_timer_divide_by_zero_register(void) {
    proc_security_register_attack(&atk);
}
