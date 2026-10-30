#ifndef DEF_FRAME_VALIDATE_H
#define DEF_FRAME_VALIDATE_H

/*
 * def_frame_validate.h - DEFENCE frame_validate: a frame must be 14-1514 bytes with a real EtherType
 */

#include <stdint.h>

#define FRAME_MIN 14            // the Ethernet header
#define FRAME_MAX 1514          // header + 1500 bytes of data

// Is a received frame of 'len' bytes with this type field well formed?
// ethernet_receive() asks. 1 = yes, 0 = drop it (logged).
int def_frame_ok(uint32_t len, uint16_t type);

#endif
