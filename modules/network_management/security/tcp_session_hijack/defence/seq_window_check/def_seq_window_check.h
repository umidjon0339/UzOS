#ifndef DEF_SEQ_WINDOW_CHECK_H
#define DEF_SEQ_WINDOW_CHECK_H

/*
 * def_seq_window_check.h - DEFENCE seq_window_check: a segment must lie in the receive window, and data must be next in line
 */

#include <stdint.h>

// Is a segment at 'seq' of 'seglen' (data + SYN + FIN) inside the
// window [rcv_nxt, rcv_nxt + wnd)? segment() asks. 1 = yes, 0 = no (logged).
int def_seq_in_window(uint32_t seq, uint32_t seglen, uint32_t rcv_nxt, uint32_t wnd);

#endif
