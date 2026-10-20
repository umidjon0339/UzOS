#ifndef GDT_H
#define GDT_H

/*
 * gdt.h - Global Descriptor Table.
 *
 * The GDT lists memory "segments" and who may use them (ring 0 or ring 3).
 * We use a flat model: every segment covers all 4 GiB, so segmentation
 * does nothing and paging does the real memory protection.
 */

#include <stdint.h>
#include "tss.h"    // set_kernel_stack(), used by the scheduler

// GDT slot numbers
#define GDT_NULL_INDEX        0
#define GDT_KERNEL_CODE_INDEX 1
#define GDT_KERNEL_DATA_INDEX 2
#define GDT_USER_CODE_INDEX   3
#define GDT_USER_DATA_INDEX   4
#define GDT_TSS_INDEX         5
#define GDT_ENTRIES           6

// Segment selectors = slot * 8, low 2 bits = requested ring (RPL).
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x1B    // 0x18 | ring 3
#define GDT_USER_DATA   0x23    // 0x20 | ring 3
#define GDT_TSS         0x28

// One 8-byte descriptor. The CPU wants base and limit split into pieces.
typedef struct {
    uint16_t limit_low;     // limit bits 0-15
    uint16_t base_low;      // base bits 0-15
    uint8_t  base_middle;   // base bits 16-23
    uint8_t  access;        // present, ring, code/data, ...
    uint8_t  granularity;   // flags (high 4 bits) + limit bits 16-19
    uint8_t  base_high;     // base bits 24-31
} __attribute__((packed)) gdt_entry_t;

// The 6-byte value the 'lgdt' instruction loads.
typedef struct {
    uint16_t limit;         // table size in bytes - 1
    uint32_t base;          // address of the first entry
} __attribute__((packed)) gdt_ptr_t;

// Build the GDT (with the TSS) and load it into the CPU.
void init_gdt(void);

// Fill in one descriptor. Used by tss.c for the TSS slot.
void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t flags);

#endif
