#ifndef CPUID_H
#define CPUID_H

/*
 * cpuid.h - Ask the CPU who it is.
 *
 * The 'cpuid' instruction takes a question number in EAX ("leaf") and
 * answers in EAX, EBX, ECX, EDX:
 *
 *   leaf 0            the vendor, 12 letters in EBX EDX ECX: "GenuineIntel"
 *   leaf 0x80000000   the highest "extended" leaf there is
 *   leaf 0x80000002-4 the brand name, 48 letters: "Intel(R) Core(TM) i7..."
 *
 * Very old CPUs (before the 486) have no cpuid: if bit 21 of EFLAGS
 * (the "ID" flag) cannot be changed, the instruction does not exist.
 *
 * This file only runs the instruction. What the answers mean (family,
 * feature flags) is decoded in cpu_management/core/info.
 */

#include <stdint.h>

// Ask question 'leaf': EAX, EBX, ECX, EDX come back in regs[0..3].
// Only call it when cpuid_available().
void cpuid_leaf(uint32_t leaf, uint32_t regs[4]);

// 1 if this CPU has the cpuid instruction.
int cpuid_available(void);

// The vendor ("GenuineIntel", "AuthenticAMD"), or "unknown".
void cpuid_vendor(char out[13]);

// The brand name, leading spaces removed. 0 if the CPU has none.
int cpuid_brand(char out[49]);

#endif
