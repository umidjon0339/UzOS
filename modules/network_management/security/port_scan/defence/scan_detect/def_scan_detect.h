#ifndef DEF_SCAN_DETECT_H
#define DEF_SCAN_DETECT_H

/*
 * def_scan_detect.h - DEFENCE scan_detect: a source that hits 10 closed ports in a second gets no more RSTs for 10 s
 */

#include <stdint.h>

#define SCAN_THRESHOLD 10       // closed ports per second before we go quiet
#define SCAN_QUIET_MS  10000

// May we answer 'src' (host order) with a RST for a closed port?
// tcp's refuse() asks. 1 = yes, 0 = a scanner: stay silent (logged once).
int def_scan_rst_ok(uint32_t src);

#endif
