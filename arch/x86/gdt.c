#include "gdt.h"

#include "string.h"

// Let's declare our GDT array with 6 entries:
// 1. Null descriptor
// 2. Kernel Code Segment
// 3. Kernel Data Segment
// 4. User Code Segment
// 5. User Data Segment
// 6. Task State Segment (TSS)
gdt_entry_t gdt_entries[6];
gdt_ptr_t   gdt_ptr;
tss_entry_t tss_entry;

// The external assembly function that loads our GDT.
extern void gdt_flush(uint32_t);

// Setup a descriptor in the Global Descriptor Table
static void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

// Initialize the TSS
static void write_tss(int32_t num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss_entry;
    uint32_t limit = base + sizeof(tss_entry);

    gdt_set_gate(num, base, limit, 0xE9, 0x00);

    memset(&tss_entry, 0, sizeof(tss_entry));

    tss_entry.ss0  = ss0;
    tss_entry.esp0 = esp0;
    
    // Here we set the cs, ss, ds, es, fs and gs entries in the TSS.
    // They specify what segments should be loaded when the processor switches to kernel mode.
    tss_entry.cs   = 0x0b;
    tss_entry.ss = tss_entry.ds = tss_entry.es = tss_entry.fs = tss_entry.gs = 0x13;
}

void set_kernel_stack(uint32_t stack) {
    tss_entry.esp0 = stack;
}

// Initialize the GDT
void init_gdt(void) {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel Code
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel Data
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User Code
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User Data
    
    write_tss(5, 0x10, 0x0); // Data segment is 0x10 (Kernel Data)

    // Flush out the old GDT and install the new changes!
    gdt_flush((uint32_t)&gdt_ptr);
    
    // Flush the TSS. We use 0x2B because 5 * 8 = 40 (0x28) + 3 (RPL)
    extern void tss_flush(void);
    tss_flush();
}
