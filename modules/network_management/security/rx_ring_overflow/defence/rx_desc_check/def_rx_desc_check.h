#ifndef DEF_RX_DESC_CHECK_H
#define DEF_RX_DESC_CHECK_H

/*
 * def_rx_desc_check.h - DEFENCE rx_desc_check (ILLUSTRATION): the driver takes only whole, error-free frames that fit their buffer
 */

#include <stdint.h>

// Is a finished RX descriptor safe to read? 'status' (bit 0 DD, bit 1
// EOP), 'errors', 'len', and the buffer's size. The e1000 driver asks.
// 1 = take the frame, 0 = drop it (logged).
int def_rx_desc_ok(uint8_t status, uint8_t errors, uint32_t len, uint32_t buffer_size);

#endif
