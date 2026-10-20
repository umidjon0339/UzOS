/*
 * cpuid.c - Run the cpuid instruction and turn its registers into text.
 */

#include "cpuid.h"
#include <stdint.h>

#define EFLAGS_ID      (1u << 21)
#define LEAF_VENDOR    0x00000000
#define LEAF_EXT_MAX   0x80000000
#define LEAF_BRAND     0x80000002   // and the two after it

void cpuid_leaf(uint32_t leaf, uint32_t regs[4]) {
    asm volatile("cpuid"
                 : "=a"(regs[0]), "=b"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
                 : "a"(leaf), "c"(0));
}

// Copy the 4 bytes of 'reg' into 'out' (x86 keeps the first letter lowest).
static void put_reg(char *out, uint32_t reg) {
    for (int i = 0; i < 4; i++)
        out[i] = (char)(reg >> (8 * i));
}

int cpuid_available(void) {
    uint32_t before, after;
    asm volatile("pushfl\n\t"               // try to flip the ID bit...
                 "pop %0\n\t"
                 "mov %0, %1\n\t"
                 "xor %2, %1\n\t"
                 "push %1\n\t"
                 "popfl\n\t"
                 "pushfl\n\t"               // ...and read EFLAGS back
                 "pop %1\n\t"
                 "push %0\n\t"              // put the old EFLAGS back
                 "popfl"
                 : "=&r"(before), "=&r"(after)
                 : "i"(EFLAGS_ID));
    return ((before ^ after) & EFLAGS_ID) != 0;
}

void cpuid_vendor(char out[13]) {
    uint32_t r[4];
    if (!cpuid_available()) {
        const char *unknown = "unknown";
        for (int i = 0; i < 8; i++)
            out[i] = unknown[i];
        return;
    }
    cpuid_leaf(LEAF_VENDOR, r);
    put_reg(out, r[1]);                     // order: EBX, EDX, ECX
    put_reg(out + 4, r[3]);
    put_reg(out + 8, r[2]);
    out[12] = '\0';
}

int cpuid_brand(char out[49]) {
    uint32_t r[4];
    if (!cpuid_available())
        return 0;
    cpuid_leaf(LEAF_EXT_MAX, r);
    if (r[0] < LEAF_BRAND + 2)
        return 0;                           // no brand string on this CPU

    char raw[49];
    for (int i = 0; i < 3; i++) {
        cpuid_leaf(LEAF_BRAND + i, r);
        for (int j = 0; j < 4; j++)
            put_reg(raw + i * 16 + j * 4, r[j]);
    }
    raw[48] = '\0';

    const char *start = raw;                // Intel pads the front with spaces
    while (*start == ' ')
        start++;
    int n = 0;
    while (start[n]) {
        out[n] = start[n];
        n++;
    }
    out[n] = '\0';
    return 1;
}
