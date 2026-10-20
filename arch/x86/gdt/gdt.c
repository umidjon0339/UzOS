/*
 * gdt.c - Build the Global Descriptor Table and load it.
 */

#include "gdt.h"
#include "tss.h"

// Access byte: what kind of segment and which ring may use it.
#define ACCESS_KERNEL_CODE 0x9A   // present, ring 0, code, readable
#define ACCESS_KERNEL_DATA 0x92   // present, ring 0, data, writable
#define ACCESS_USER_CODE   0xFA   // present, ring 3, code, readable
#define ACCESS_USER_DATA   0xF2   // present, ring 3, data, writable

// Flags: limit counted in 4 KiB pages, 32-bit segment.
#define FLAGS_4K_32BIT     0xC0

static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t   gdt_ptr;

// From gdt_load.s
extern void gdt_load(gdt_ptr_t *ptr);

void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                   uint8_t access, uint8_t flags) {
    gdt_entry_t *e = &gdt[index];

    e->base_low    = base & 0xFFFF;
    e->base_middle = (base >> 16) & 0xFF;
    e->base_high   = (base >> 24) & 0xFF;

    e->limit_low   = limit & 0xFFFF;
    e->granularity = ((limit >> 16) & 0x0F) | (flags & 0xF0);

    e->access      = access;
}

void init_gdt(void) {
    // Flat segments: base 0, limit 0xFFFFF pages = 4 GiB.
    gdt_set_entry(GDT_NULL_INDEX,        0, 0,       0, 0);   // CPU requires slot 0 empty
    gdt_set_entry(GDT_KERNEL_CODE_INDEX, 0, 0xFFFFF, ACCESS_KERNEL_CODE, FLAGS_4K_32BIT);
    gdt_set_entry(GDT_KERNEL_DATA_INDEX, 0, 0xFFFFF, ACCESS_KERNEL_DATA, FLAGS_4K_32BIT);
    gdt_set_entry(GDT_USER_CODE_INDEX,   0, 0xFFFFF, ACCESS_USER_CODE,   FLAGS_4K_32BIT);
    gdt_set_entry(GDT_USER_DATA_INDEX,   0, 0xFFFFF, ACCESS_USER_DATA,   FLAGS_4K_32BIT);

    // Slot 5 describes the TSS (see arch/x86/tss/).
    tss_init(GDT_TSS_INDEX);

    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    gdt_load(&gdt_ptr);     // switch the CPU to our new segments
    tss_load();             // then tell it where the TSS is
}
