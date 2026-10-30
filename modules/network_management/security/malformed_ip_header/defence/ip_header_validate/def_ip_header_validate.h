#ifndef DEF_IP_HEADER_VALIDATE_H
#define DEF_IP_HEADER_VALIDATE_H

/*
 * def_ip_header_validate.h - DEFENCE ip_header_validate: version 4, 20 <= IHL*4 <= total length <= packet, and a correct checksum
 */

#include <stdint.h>

// Is the IPv4 header at 'bytes' (len bytes received) sane? ipv4_receive()
// asks. 1 = yes, 0 = drop (logged).
int def_ip_header_ok(const uint8_t *bytes, uint32_t len);

#endif
