/*
 * ksyms.c - Find the function an address belongs to.
 *
 * The table itself is generated (build/ksyms.c). During the FIRST link
 * of the kernel it does not exist yet, so the two names below are
 * "weak": a missing weak symbol simply has the address 0, and every
 * lookup then answers "unknown". The second link supplies the real table.
 *
 * Uses: _text_start / _text_end (boot/linker.ld): where kernel code is.
 */

#include "modules/cpu_management/commands/perf_report/ksyms.h"   // ksym_t, this file's functions

extern const ksym_t   ksym_table[] __attribute__((weak));
extern const uint32_t ksym_count     __attribute__((weak));
extern char _text_end[];                // boot/linker.ld: end of the kernel's code

uint32_t ksyms_count(void) {
    return &ksym_count ? ksym_count : 0;
}

const char *ksyms_lookup(uint32_t address, uint32_t *offset) {
    uint32_t n = ksyms_count();
    if (n == 0 || address < ksym_table[0].address || address >= (uint32_t)_text_end)
        return 0;

    // Binary search for the last function that starts at or before 'address'.
    uint32_t low = 0, high = n - 1;
    while (low < high) {
        uint32_t mid = (low + high + 1) / 2;
        if (ksym_table[mid].address <= address)
            low = mid;
        else
            high = mid - 1;
    }
    if (offset)
        *offset = address - ksym_table[low].address;
    return ksym_table[low].name;
}
