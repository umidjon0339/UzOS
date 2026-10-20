#ifndef IDT_H
#define IDT_H

/*
 * idt.h - Interrupt Descriptor Table.
 *
 * 256 entries, one per interrupt vector. Each entry ("gate") says
 * which function the CPU jumps to and which rings may trigger it.
 *   0-31   CPU exceptions (divide by zero, page fault, ...)
 *   32-47  hardware IRQs (timer, keyboard, ...) after PIC remap
 *   128    system call (int 0x80)
 */

#include <stdint.h>

#define IDT_ENTRIES 256

// One 8-byte gate.
typedef struct {
    uint16_t base_lo;       // handler address bits 0-15
    uint16_t sel;           // code segment to run the handler in
    uint8_t  always0;
    uint8_t  flags;         // present, ring, gate type
    uint16_t base_hi;       // handler address bits 16-31
} __attribute__((packed)) idt_entry_t;

// The 6-byte value the 'lidt' instruction loads.
typedef struct {
    uint16_t limit;         // table size in bytes - 1
    uint32_t base;          // address of the first gate
} __attribute__((packed)) idt_ptr_t;

// Point every vector at its handler stub and load the IDT.
void init_idt(void);

#endif
