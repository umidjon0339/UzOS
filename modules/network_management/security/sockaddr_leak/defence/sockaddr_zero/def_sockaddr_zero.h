#ifndef DEF_SOCKADDR_ZERO_H
#define DEF_SOCKADDR_ZERO_H

/*
 * def_sockaddr_zero.h - DEFENCE sockaddr_zero: every address handed to a program is wiped before it is filled in
 */

#include <stdint.h>

// Wipe 'size' bytes of an address the kernel is about to fill.
void def_sockaddr_wipe(void *addr, uint32_t size);

#endif
