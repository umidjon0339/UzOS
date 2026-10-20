/*
 * atk_rtc_garbage.c - ATTACK rtc_garbage: the clock chip returns nonsense.
 *
 * Breaks:     trust - data from hardware is checked before use
 * Stopped by: rtc_validate (security/defence/rtc_validate)
 *
 * A missing RTC answers 0xFF to every read. Decoded as BCD that is
 * 15*10 + 15 = 165: month 165, hour 165. A table lookup with month 165
 * (rtc_month_name, rtc_unix_time) reads memory far past the table.
 *
 * simulate(): show what 0xFF registers decode to without a check.
 * launch():   feed the real decoder three bad register sets: all 0xFF,
 *             month 13, and 30 February. Every one must be refused.
 *
 * Uses: rtc_decode / rtc_read (rtc/).
 */

#include "security_lab.h"
#include "rtc.h"
#include "vga.h"

#define BCD_24H 0x02            // status B: BCD values, 24-hour clock

static const struct {
    rtc_raw_t raw;
    const char *what;
} bad[] = {
    { { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, BCD_24H }, "no chip: every register 0xFF" },
    { { 0x00, 0x00, 0x12, 0x01, 0x13, 0x26, BCD_24H }, "month 13" },
    { { 0x00, 0x00, 0x12, 0x30, 0x02, 0x26, BCD_24H }, "30 February" },
};
#define BAD_COUNT (sizeof(bad) / sizeof(bad[0]))

static int simulate(void) {
    vga_print_str("  registers read 0xFF; as BCD: 0xFF -> 15*10 + 15 = 165\n");
    vga_print_str("  date would be 2165-165-165 165:165:165\n");
    vga_print_str("  rtc_month_name(165) -> reads entry 164 of a 12-entry table\n");
    return 1;
}

static int launch(void) {
    uint32_t denied = sec_total_denied();
    rtc_time_t t;

    for (unsigned i = 0; i < BAD_COUNT; i++) {
        vga_print_str("  ");
        vga_print_str(bad[i].what);
        vga_print_str(" -> ");
        vga_print_str(rtc_decode(&bad[i].raw, &t) ? "ACCEPTED\n" : "refused\n");
    }

    vga_print_str("  the real clock still reads: ");
    vga_print_str(rtc_read(&t) ? "a valid date\n" : "nothing valid\n");
    return sec_total_denied() - denied;
}

static const attack_t attack = {
    "rtc_garbage",
    "a missing or broken RTC returns impossible dates (month 165)",
    INV_TRUST, SEV_MEDIUM,
    simulate, launch, 0,        // only decoding was tried: nothing to undo
    "rtc_validate"
};

void atk_rtc_garbage_register(void) {
    proc_security_register_attack(&attack);
}
