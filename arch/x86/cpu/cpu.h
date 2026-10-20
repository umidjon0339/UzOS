#ifndef CPU_H
#define CPU_H

/*
 * cpu.h - Small CPU helpers: interrupts off/on, read ESP, control
 *         registers, the cycle counter.
 *
 * Code that must not be interrupted halfway (like a task switch) does:
 *
 *     uint32_t flags = cpu_irq_save();    // remember IF, then cli
 *     ... critical work ...
 *     cpu_irq_restore(flags);             // IF back to what it was
 *
 * Restoring (not just 'sti') matters: if interrupts were already off,
 * they must stay off.
 */

#include <stdint.h>

#define EFLAGS_IF 0x200     // Interrupt Flag: 1 = interrupts allowed

// Disable interrupts and return the old EFLAGS.
static inline uint32_t cpu_irq_save(void) {
    uint32_t flags;
    asm volatile("pushf; pop %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

// Re-enable interrupts only if they were on before cpu_irq_save().
static inline void cpu_irq_restore(uint32_t flags) {
    if (flags & EFLAGS_IF)
        asm volatile("sti" : : : "memory");
}

// The current stack pointer.
static inline uint32_t cpu_read_esp(void) {
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    return esp;
}

/* ---------- Control registers ----------
 * CR0 and CR4 are switches that change how the whole CPU behaves.
 * Only ring 0 may read or write them. */

#define CR0_PE  (1u << 0)   // Protection Enable: protected mode (GRUB turns it on)
#define CR0_EM  (1u << 2)   // Emulation: FPU instructions raise exception 7
#define CR0_TS  (1u << 3)   // Task Switched: next FPU instruction raises exception 7
#define CR0_WP  (1u << 16)  // Write Protect: ring 0 obeys read-only pages too
#define CR0_PG  (1u << 31)  // Paging on (memory_management/core/vmm)

#define CR4_VME (1u << 0)   // virtual 8086 mode extensions
#define CR4_TSD (1u << 2)   // Time Stamp Disable: rdtsc only in ring 0
#define CR4_PSE (1u << 4)   // 4 MiB pages allowed
#define CR4_PAE (1u << 5)   // 64-bit page table entries
#define CR4_PGE (1u << 7)   // global pages survive a CR3 reload

static inline uint32_t cpu_read_cr0(void) {
    uint32_t value;
    asm volatile("mov %%cr0, %0" : "=r"(value));
    return value;
}

static inline uint32_t cpu_read_cr4(void) {
    uint32_t value;
    asm volatile("mov %%cr4, %0" : "=r"(value));
    return value;
}

static inline void cpu_write_cr4(uint32_t value) {
    asm volatile("mov %0, %%cr4" : : "r"(value) : "memory");
}

/* ---------- Cycle counter ----------
 * The Time Stamp Counter (TSC) counts CPU clock cycles since power on.
 * 'rdtsc' puts the 64-bit count in EDX:EAX. Check cpuid's TSC flag
 * before using it (cpu_management/core/info). */

static inline uint64_t cpu_rdtsc(void) {
    uint32_t low, high;
    asm volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

#endif
