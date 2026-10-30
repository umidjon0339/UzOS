#ifndef DEF_RST_WINDOW_CHECK_H
#define DEF_RST_WINDOW_CHECK_H

/*
 * def_rst_window_check.h - DEFENCE rst_window_check: a RST resets only at exactly the next expected sequence number
 */

#include <stdint.h>

// Does a RST with sequence 'seq' really end a connection expecting
// 'rcv_nxt'? segment() asks. 1 = reset, 0 = challenge ACK (logged).
int def_rst_exact_ok(uint32_t seq, uint32_t rcv_nxt);

#endif
