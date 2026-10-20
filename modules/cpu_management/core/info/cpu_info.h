#ifndef CPU_INFO_H
#define CPU_INFO_H

/*
 * cpu_info.h - Who is the CPU, and what can it do?
 *
 * The cpuid instruction (arch/x86/cpuid) answers in raw register bits.
 * This file turns them into something readable, once at boot:
 *
 *   leaf 0    vendor        "GenuineIntel"
 *   leaf 1    EAX           family / model / stepping: which chip design
 *             EDX, ECX      one bit per FEATURE: "has a TSC", "has PAE", ...
 *
 * The rest of the kernel asks cpu_has(CPU_FEAT_TSC) instead of
 * decoding bits itself. A kernel must never use a feature the CPU does
 * not have: the instruction would raise "Invalid Opcode" (exception 6).
 */

#include <stdint.h>

// The features UzOS knows about. The order is the order `cpuinfo` shows.
typedef enum {
    CPU_FEAT_FPU,           // x87 floating point unit on the chip
    CPU_FEAT_TSC,           // Time Stamp Counter: rdtsc
    CPU_FEAT_MSR,           // model-specific registers: rdmsr / wrmsr
    CPU_FEAT_PSE,           // 4 MiB pages
    CPU_FEAT_PAE,           // physical address extension (> 4 GiB RAM)
    CPU_FEAT_PGE,           // global pages
    CPU_FEAT_APIC,          // local APIC: the PIC's modern successor
    CPU_FEAT_SEP,           // sysenter / sysexit: faster syscalls than int 0x80
    CPU_FEAT_CMOV,          // conditional move instructions
    CPU_FEAT_MMX,           // MMX vector instructions
    CPU_FEAT_FXSR,          // fxsave / fxrstor: save FPU + SSE state
    CPU_FEAT_SSE,           // SSE vector instructions
    CPU_FEAT_SSE2,
    CPU_FEAT_SSE3,
    CPU_FEAT_HTT,           // more than one logical CPU in this package
    CPU_FEAT_HYPERVISOR,    // running inside a virtual machine (QEMU, ...)
    CPU_FEAT_COUNT
} cpu_feature_t;

typedef struct {
    char     vendor[13];    // "GenuineIntel", "AuthenticAMD", "unknown"
    char     brand[49];     // "QEMU Virtual CPU version 2.5+", or ""
    uint32_t family;        // chip generation (6 = Pentium Pro and later)
    uint32_t model;         // design inside the family
    uint32_t stepping;      // revision of that design
    uint32_t edx, ecx;      // raw feature bits from leaf 1
} cpu_info_t;

// Ask the CPU everything once. Called from kernel_main before anything
// that depends on a feature.
void cpu_info_init(void);

const cpu_info_t *cpu_info(void);

// 1 if the CPU has feature 'f'.
int cpu_has(cpu_feature_t f);

// Short name ("tsc") and one-line meaning, for `cpuinfo`.
const char *cpu_feature_name(cpu_feature_t f);
const char *cpu_feature_meaning(cpu_feature_t f);

// How many of the known features this CPU has.
int cpu_feature_present_count(void);

#endif
