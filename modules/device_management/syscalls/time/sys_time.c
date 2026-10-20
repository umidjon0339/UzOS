/*
 * sys_time.c - time(): read the real-time clock for a program.
 *
 * Programs cannot use the CMOS ports (they are kernel-only), so they
 * ask the kernel. One number is easy to pass in a register, and easy to
 * subtract: two times give the seconds between them.
 *
 * Uses: rtc_read / rtc_unix_time (rtc/).
 */

#include "sys_time.h"
#include "rtc.h"

uint32_t sys_time(void) {
    rtc_time_t now;
    return rtc_read(&now) ? rtc_unix_time(&now) : 0;
}
