#ifndef DEF_SYN_COOKIES_H
#define DEF_SYN_COOKIES_H

/*
 * def_syn_cookies.h - DEFENCE syn_cookies: a full listen queue answers with SYN cookies instead of dropping
 */

#include <stdint.h>

// The cookie (our ISN) for a SYN on (local, remote) with the client's
// sequence number 'their_seq', remembering 'mss' as one of 8 sizes.
uint32_t def_syn_cookie_make(uint32_t local_ip, uint16_t local_port, uint32_t remote_ip,
                             uint16_t remote_port, uint32_t their_seq, uint16_t mss);

// Is 'cookie' (the ACK's ack - 1) one we made for this connection (now,
// or in the last 64 s)? The MSS it carries, or 0 if it is not ours (logged).
uint16_t def_syn_cookie_check(uint32_t local_ip, uint16_t local_port, uint32_t remote_ip,
                              uint16_t remote_port, uint32_t their_seq, uint32_t cookie);

#endif
