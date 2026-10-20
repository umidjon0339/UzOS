#ifndef IO_H
#define IO_H

/*
 * io.h - Talk to hardware through I/O ports.
 *
 * x86 has a separate 64K "port" address space for devices
 * (PIC at 0x20, keyboard at 0x60, PIT at 0x40, ...).
 * Only the 'in' and 'out' instructions can reach it.
 */

#include <stdint.h>

// Write one byte to a port.
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Read one byte from a port.
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Write a 16-bit word to a port.
static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

// Read a 16-bit word from a port.
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Wait ~1-4 microseconds by writing to unused port 0x80.
// Old devices (like the PIC) need a pause between commands.
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif
