#include <stdint.h>
#include "device_security.h"
#include "proc_security.h"

int device_timer_frequency_valid(uint32_t frequency) {
    if (frequency < PIT_MIN_FREQUENCY || frequency > PIT_INPUT_HZ) return 0;
    return PIT_INPUT_HZ / frequency <= 0xFFFFu;
}

static int def_timer_frequency_selftest(void) {
    if (device_timer_frequency_valid(0)) return 0;
    if (device_timer_frequency_valid(PIT_MIN_FREQUENCY - 1)) return 0;
    if (!device_timer_frequency_valid(PIT_MIN_FREQUENCY)) return 0;
    if (!device_timer_frequency_valid(100)) return 0;
    if (!device_timer_frequency_valid(PIT_INPUT_HZ)) return 0;
    if (device_timer_frequency_valid(PIT_INPUT_HZ + 1)) return 0;
    if (device_timer_frequency_valid(0xFFFFFFFFu)) return 0;
    return 1;
}

static const defence_t def = {
    "timer_frequency",
    "reject PIT frequencies whose divisor cannot fit in the PIT counter",
    "timer_divide_by_zero",
    def_timer_frequency_selftest
};

void def_timer_frequency_register(void) {
    proc_security_register_defence(&def);
}
