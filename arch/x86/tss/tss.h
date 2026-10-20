#ifndef TSS_H
#define TSS_H

/*
 * tss.h - Task State Segment.
 *
 * When an interrupt arrives while user code (ring 3) runs, the CPU must
 * switch to a kernel stack. It reads that stack from the TSS (ss0:esp0).
 * We only use these two fields; no hardware task switching.
 */

#include <stdint.h>

// Layout fixed by the CPU (104 bytes).
typedef struct {
    uint32_t prev_tss;
    uint32_t esp0;          // kernel stack pointer used on ring 3 -> ring 0
    uint32_t ss0;           // kernel stack segment
    uint32_t esp1, ss1;     // rings 1 and 2: unused
    uint32_t esp2, ss2;
    uint32_t cr3;           // fields below are for hardware task switching
    uint32_t eip, eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;    // offset of the I/O port permission bitmap
} __attribute__((packed)) tss_entry_t;

// Fill the TSS and put its descriptor in GDT slot 'gdt_index'.
void tss_init(int gdt_index);

// Load the Task Register (call after the GDT is loaded).
void tss_load(void);

// Set the kernel stack for the next ring 3 -> ring 0 switch.
// The scheduler calls this on every task switch.
void set_kernel_stack(uint32_t stack);

#endif
