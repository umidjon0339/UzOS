#ifndef RTC_H
#define RTC_H

/*
 * rtc.h - Real-Time Clock: the date and time, even after power-off.
 *
 * The RTC is a tiny clock chip with its own battery, inside the "CMOS"
 * memory of the PC. The timer (timer/) only counts ticks since boot;
 * the RTC knows the actual calendar date.
 *
 * It is read through two ports: write a register number to 0x70, read
 * its value from 0x71.
 *
 *   register  0x00 seconds   0x02 minutes   0x04 hours
 *             0x07 day       0x08 month     0x09 year (two digits)
 *             0x0A status A  (bit 7: "busy updating, don't read now")
 *             0x0B status B  (bit 2: binary, not BCD; bit 1: 24-hour)
 *
 * Values are usually BCD: 0x59 means 59, one decimal digit per 4 bits.
 */

#include <stdint.h>

typedef struct {
    uint16_t year;          // 2026
    uint8_t  month;         // 1-12
    uint8_t  day;           // 1-31
    uint8_t  hour;          // 0-23
    uint8_t  minute;        // 0-59
    uint8_t  second;        // 0-59
    uint8_t  weekday;       // 0 = Sunday ... 6 = Saturday
} rtc_time_t;

// The registers as the chip gives them, before any decoding.
typedef struct {
    uint8_t second, minute, hour, day, month, year;
    uint8_t status_b;
} rtc_raw_t;

// Read the clock. 1 = 't' is valid; 0 = the chip gave an impossible
// date (refused by rtc_validate, 't' is left alone).
int rtc_read(rtc_time_t *t);

// Turn raw registers into a date (BCD, 12-hour clock...) and check it.
// rtc_read() uses this; the security lab feeds it bad values.
int rtc_decode(const rtc_raw_t *raw, rtc_time_t *t);

// Seconds since 1970-01-01 00:00:00 ("Unix time").
uint32_t rtc_unix_time(const rtc_time_t *t);

const char *rtc_weekday_name(int weekday);      // "Mon"
const char *rtc_month_name(int month);          // "Oct"

#endif
