#ifndef DEF_RTC_VALIDATE_H
#define DEF_RTC_VALIDATE_H

#include "rtc.h"

// Is 't' a real date and time? 1 = yes, 0 = refused (logged).
int def_rtc_time_ok(const rtc_time_t *t);

#endif
