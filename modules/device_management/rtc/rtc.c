/*
 * rtc.c - Read and decode the CMOS real-time clock.
 *
 * Reading safely: the chip updates its registers once a second, and
 * reading in the middle of an update could give e.g. 14:59 + the new
 * hour = 15:59. So: wait until it is not updating, read everything,
 * read everything again, and repeat until both reads agree.
 *
 * Uses: outb/inb (arch/x86/io), cpu_irq_save (arch/x86/cpu),
 *       def_rtc_time_ok (defence rtc_validate).
 */

#include "rtc.h"
#include "io.h"
#include "cpu.h"
#include "def_rtc_validate.h"

#define CMOS_INDEX  0x70
#define CMOS_DATA   0x71

#define REG_SECOND  0x00
#define REG_MINUTE  0x02
#define REG_HOUR    0x04
#define REG_DAY     0x07
#define REG_MONTH   0x08
#define REG_YEAR    0x09
#define REG_STATUS_A 0x0A
#define REG_STATUS_B 0x0B

#define UPDATING     0x80   // status A: the chip is changing the registers
#define BINARY_MODE  0x04   // status B: values are binary, not BCD
#define HOUR_24      0x02   // status B: 24-hour clock
#define HOUR_PM      0x80   // in 12-hour mode: bit 7 of the hour = PM

static uint8_t cmos_read(uint8_t reg) {
    outb(CMOS_INDEX, reg);
    return inb(CMOS_DATA);
}

#define MAX_WAIT 100000             // an update takes ~2 ms; no chip = forever

static void read_raw(rtc_raw_t *raw) {
    for (int i = 0; i < MAX_WAIT && (cmos_read(REG_STATUS_A) & UPDATING); i++)
        ;
    raw->second = cmos_read(REG_SECOND);
    raw->minute = cmos_read(REG_MINUTE);
    raw->hour   = cmos_read(REG_HOUR);
    raw->day    = cmos_read(REG_DAY);
    raw->month  = cmos_read(REG_MONTH);
    raw->year   = cmos_read(REG_YEAR);
    raw->status_b = cmos_read(REG_STATUS_B);
}

static int same(const rtc_raw_t *a, const rtc_raw_t *b) {
    return a->second == b->second && a->minute == b->minute && a->hour == b->hour &&
           a->day == b->day && a->month == b->month && a->year == b->year;
}

// 0x59 -> 59. A "digit" above 9 gives a number that fails the check later.
static uint8_t from_bcd(uint8_t v) {
    return (v >> 4) * 10 + (v & 0x0F);
}

// Day of the week, 0 = Sunday (Sakamoto's method).
static int weekday_of(int y, int m, int d) {
    static const int offset[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    if (m < 3)
        y--;
    return (y + y / 4 - y / 100 + y / 400 + offset[m - 1] + d) % 7;
}

int rtc_decode(const rtc_raw_t *raw, rtc_time_t *t) {
    int binary = raw->status_b & BINARY_MODE;
    int pm = !(raw->status_b & HOUR_24) && (raw->hour & HOUR_PM);
    uint8_t hour = raw->hour & ~HOUR_PM;

    rtc_time_t out;
    out.second = binary ? raw->second : from_bcd(raw->second);
    out.minute = binary ? raw->minute : from_bcd(raw->minute);
    out.hour   = binary ? hour        : from_bcd(hour);
    out.day    = binary ? raw->day    : from_bcd(raw->day);
    out.month  = binary ? raw->month  : from_bcd(raw->month);
    out.year   = 2000 + (binary ? raw->year : from_bcd(raw->year));

    if (!(raw->status_b & HOUR_24)) {   // 12-hour clock: 12 AM = 0, 1 PM = 13
        if (out.hour == 12) out.hour = 0;
        if (pm) out.hour += 12;
    }

    if (!def_rtc_time_ok(&out))         // never pass on an impossible date
        return 0;
    out.weekday = weekday_of(out.year, out.month, out.day);
    *t = out;
    return 1;
}

int rtc_read(rtc_time_t *t) {
    rtc_raw_t a, b;
    uint32_t irq = cpu_irq_save();      // the index/data port pair is shared
    read_raw(&b);
    do {
        a = b;
        read_raw(&b);
    } while (!same(&a, &b));            // an update happened in between: again
                                        // (no chip reads 0xFF twice: stops too)
    cpu_irq_restore(irq);
    return rtc_decode(&b, t);
}

uint32_t rtc_unix_time(const rtc_time_t *t) {
    static const uint16_t days_before[] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    uint32_t days = 0;

    for (int y = 1970; y < t->year; y++)
        days += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
    days += days_before[t->month - 1] + t->day - 1;
    int leap = t->year % 4 == 0 && (t->year % 100 != 0 || t->year % 400 == 0);
    if (leap && t->month > 2)
        days++;                         // 29 February has passed

    return days * 86400 + t->hour * 3600 + t->minute * 60 + t->second;
}

const char *rtc_weekday_name(int weekday) {
    static const char *names[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    return (weekday >= 0 && weekday < 7) ? names[weekday] : "???";
}

const char *rtc_month_name(int month) {
    static const char *names[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    return (month >= 1 && month <= 12) ? names[month - 1] : "???";
}
