/*
 * def_rtc_validate.c - DEFENCE rtc_validate: only real dates leave the RTC driver.
 *
 * Stops:  rtc_garbage (security/attack/rtc_garbage)
 * Called: rtc_decode() (rtc/), after decoding, before anyone sees the date.
 * Rule:   2000 <= year <= 2099, month 1-12, day 1..days in that month,
 *         hour < 24, minute < 60, second < 60
 * Why:    the RTC is hardware outside the kernel's control: a dead
 *         battery, a missing chip (every read gives 0xFF) or a broken
 *         emulator can return anything. Code after it trusts the date:
 *         rtc_month_name(month) indexes a table, rtc_unix_time() uses
 *         month - 1 as an index. Month 165 would read far past the table.
 */

#include "def_rtc_validate.h"
#include "security_lab.h"

static int days_in_month(int year, int month) {
    static const uint8_t days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    return month == 2 && leap ? 29 : days[month - 1];
}

// The whole rule, as a pure function so the selftest can try it.
static int time_ok(const rtc_time_t *t) {
    if (t->year < 2000 || t->year > 2099) return 0;
    if (t->month < 1 || t->month > 12) return 0;        // checked before the table lookup
    if (t->day < 1 || t->day > days_in_month(t->year, t->month)) return 0;
    return t->hour < 24 && t->minute < 60 && t->second < 60;
}

int def_rtc_time_ok(const rtc_time_t *t) {
    if (time_ok(t))
        return 1;
    sec_log("rtc_validate", "impossible date/time from the RTC refused");
    return 0;
}

static int selftest(void) {
    rtc_time_t good   = { 2026, 10, 19, 14, 32, 5, 0 };
    rtc_time_t leap   = { 2024, 2, 29, 0, 0, 0, 0 };
    rtc_time_t noleap = { 2026, 2, 29, 0, 0, 0, 0 };
    rtc_time_t month  = { 2026, 165, 1, 0, 0, 0, 0 };   // 0xFF decoded as BCD
    rtc_time_t zero   = { 2026, 0, 0, 0, 0, 0, 0 };
    rtc_time_t hour   = { 2026, 1, 1, 24, 0, 0, 0 };
    rtc_time_t second = { 2026, 1, 1, 0, 0, 60, 0 };

    return time_ok(&good) && time_ok(&leap) && !time_ok(&noleap) &&
           !time_ok(&month) && !time_ok(&zero) && !time_ok(&hour) && !time_ok(&second);
}

static const defence_t defence = {
    "rtc_validate",
    "refuse any date/time from the RTC that cannot exist",
    "rtc_garbage",
    selftest
};

void def_rtc_validate_register(void) {
    proc_security_register_defence(&defence);
}
