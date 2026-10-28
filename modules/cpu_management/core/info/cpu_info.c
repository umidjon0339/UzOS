/*
 * cpu_info.c - Decode cpuid's answers into cpu_info_t.
 *
 * Leaf 1 EAX holds the chip's "signature":
 *
 *   bits 27-20  extended family   bits 19-16  extended model
 *   bits 11-8   family            bits  7-4   model       bits 3-0  stepping
 *
 * The extended parts only count for the newest families (Intel's rule):
 * family 15 adds the extended family, family 6 and 15 add the extended
 * model as the high 4 bits.
 *
 * Uses: cpuid_available / cpuid_leaf / cpuid_vendor / cpuid_brand
 *       (arch/x86/cpuid).
 */

#include "modules/cpu_management/core/info/cpu_info.h"
#include "arch/x86/cpuid/cpuid.h"

#define LEAF_FEATURES 1

enum { REG_EDX, REG_ECX };

// Where each feature's bit lives in leaf 1, in cpu_feature_t order.
static const struct {
    const char *name;
    int reg;
    int bit;
    const char *meaning;
} features[CPU_FEAT_COUNT] = {
    [CPU_FEAT_FPU]        = { "fpu",  REG_EDX,  0, "x87 floating point unit" },
    [CPU_FEAT_TSC]        = { "tsc",  REG_EDX,  4, "cycle counter (rdtsc)" },
    [CPU_FEAT_MSR]        = { "msr",  REG_EDX,  5, "model-specific registers" },
    [CPU_FEAT_PSE]        = { "pse",  REG_EDX,  3, "4 MiB pages" },
    [CPU_FEAT_PAE]        = { "pae",  REG_EDX,  6, "more than 4 GiB of RAM" },
    [CPU_FEAT_PGE]        = { "pge",  REG_EDX, 13, "global pages" },
    [CPU_FEAT_APIC]       = { "apic", REG_EDX,  9, "local APIC interrupt chip" },
    [CPU_FEAT_SEP]        = { "sep",  REG_EDX, 11, "sysenter fast syscalls" },
    [CPU_FEAT_CMOV]       = { "cmov", REG_EDX, 15, "conditional moves" },
    [CPU_FEAT_MMX]        = { "mmx",  REG_EDX, 23, "MMX vector instructions" },
    [CPU_FEAT_FXSR]       = { "fxsr", REG_EDX, 24, "save FPU/SSE state" },
    [CPU_FEAT_SSE]        = { "sse",  REG_EDX, 25, "SSE vector instructions" },
    [CPU_FEAT_SSE2]       = { "sse2", REG_EDX, 26, "SSE2 vector instructions" },
    [CPU_FEAT_SSE3]       = { "sse3", REG_ECX,  0, "SSE3 vector instructions" },
    [CPU_FEAT_HTT]        = { "htt",  REG_EDX, 28, "several logical CPUs" },
    [CPU_FEAT_HYPERVISOR] = { "hv",   REG_ECX, 31, "running in a virtual machine" },
};

static cpu_info_t info;

void cpu_info_init(void) {
    cpuid_vendor(info.vendor);              // "unknown" without cpuid
    if (!cpuid_available())
        return;                             // a 386/486: no family, no features
    if (!cpuid_brand(info.brand))
        info.brand[0] = '\0';

    uint32_t r[4];
    cpuid_leaf(LEAF_FEATURES, r);
    uint32_t sig = r[0];

    info.stepping = sig & 0xF;
    info.model    = (sig >> 4) & 0xF;
    info.family   = (sig >> 8) & 0xF;
    if (info.family == 15)
        info.family += (sig >> 20) & 0xFF;
    if (info.family == 6 || info.family >= 15)
        info.model += ((sig >> 16) & 0xF) << 4;

    info.edx = r[3];
    info.ecx = r[2];
}

const cpu_info_t *cpu_info(void) {
    return &info;
}

int cpu_has(cpu_feature_t f) {
    if (f >= CPU_FEAT_COUNT)
        return 0;
    uint32_t bits = features[f].reg == REG_EDX ? info.edx : info.ecx;
    return (bits >> features[f].bit) & 1;
}

const char *cpu_feature_name(cpu_feature_t f) {
    return f < CPU_FEAT_COUNT ? features[f].name : "?";
}

const char *cpu_feature_meaning(cpu_feature_t f) {
    return f < CPU_FEAT_COUNT ? features[f].meaning : "?";
}

int cpu_feature_present_count(void) {
    int n = 0;
    for (int f = 0; f < CPU_FEAT_COUNT; f++)
        n += cpu_has(f);
    return n;
}
