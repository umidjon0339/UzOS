/*
 * tss.c - Set up the single TSS used for ring 3 -> ring 0 stack switches.
 */

#include "tss.h"
#include "gdt.h"

// Access byte: present, ring 0, type 9 = 32-bit available TSS.
// Ring 0 so user code cannot jump into the TSS.
#define ACCESS_TSS 0x89

static tss_entry_t tss;

// Top of the boot stack (boot/boot.s).
extern uint8_t stack_top[];

void tss_init(int gdt_index) {
    // Limit = last valid byte, so size - 1.
    gdt_set_entry(gdt_index, (uint32_t)&tss, sizeof(tss) - 1, ACCESS_TSS, 0);

    // The tss lives in .bss, so every other field is already 0.
    tss.ss0  = GDT_KERNEL_DATA;
    tss.esp0 = (uint32_t)stack_top;     // boot stack until the scheduler runs

    // Bitmap offset past the end of the TSS = "no bitmap", so ring 3
    // can never use in/out. (Offset 0 would read the TSS itself as the
    // bitmap and allow some ports.)
    tss.iomap_base = sizeof(tss);
}

void tss_load(void) {
    // 'ltr' tells the CPU which GDT entry is the TSS.
    asm volatile("ltr %0" : : "r"((uint16_t)GDT_TSS));
}

void set_kernel_stack(uint32_t stack) {
    tss.esp0 = stack;
}
