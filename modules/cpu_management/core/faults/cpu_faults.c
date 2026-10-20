/*
 * cpu_faults.c - Catch every CPU exception and apply the fault policy.
 *
 * The flow for an exception, e.g. a ring 3 program doing `div` by 0:
 *
 *   CPU raises #0 -> isr0 stub -> isr_handler() -> fault_handler(regs)
 *       fault_isolation: from ring 3?  yes -> record it, process_exit()
 *                                      no  -> isr_panic(): halt
 *
 * To make the report useful we also look at the bytes at EIP and name
 * the instruction, when it is one of the usual suspects.
 *
 * Uses: register_interrupt_handler / isr_panic / isr_exception_name
 *       (arch/x86/isr), process_exit + the current process
 *       (process_management), def_user_range_ok (memory_management
 *       defence user_ptr_bounds) to read the program's code safely,
 *       timer_ticks (device_management), ui_* and the defence safe_text
 *       (ui), and the defences fault_isolation and tsc_restrict.
 */

#include "cpu_faults.h"
#include "scheduler.h"
#include "timer.h"
#include "string.h"
#include "ui.h"

#include "def_fault_isolation.h"
#include "def_tsc_restrict.h"
#include "def_user_ptr_bounds.h"
#include "def_safe_text.h"

#define PAGE_FAULT 14       // memory_management/core/vmm owns this one

static uint32_t kills[ISR_EXCEPTIONS];
static uint32_t total;

static cpu_fault_t log_ring[CPU_FAULT_LOG_SIZE];
static int log_head, log_count;

/* ---------- What each exception means ---------- */

static const char *const hints[ISR_EXCEPTIONS] = {
    [0]  = "divided by zero, or the result did not fit",
    [1]  = "single-step or debug register hit",
    [2]  = "hardware error signal: machine is broken",
    [3]  = "int3 breakpoint",
    [4]  = "'into' found the overflow flag set",
    [5]  = "'bound' found an index outside its range",
    [6]  = "the CPU does not know this instruction",
    [7]  = "FPU instruction while the FPU is off",
    [8]  = "a fault while handling a fault",
    [10] = "the TSS is broken",
    [11] = "a segment that is not present",
    [12] = "the stack segment is wrong or overflowed",
    [13] = "privileged instruction, port or segment",
    [14] = "address not mapped, or not allowed",
    [16] = "x87 floating point error",
    [17] = "misaligned access with checks on",
    [18] = "the CPU found a hardware error",
    [19] = "SSE floating point error",
};

const char *cpu_fault_hint(uint8_t vector) {
    if (vector >= ISR_EXCEPTIONS || !hints[vector])
        return "reserved by Intel";
    return hints[vector];
}

/* ---------- Naming the instruction at EIP ---------- */

static const struct {
    uint8_t bytes[2];
    int length;                     // 1 or 2 bytes to compare
    const char *name;
} known[] = {
    { { 0xFA },       1, "cli" },           // interrupts off
    { { 0xFB },       1, "sti" },           // interrupts on
    { { 0xF4 },       1, "hlt" },           // stop the CPU
    { { 0xE4 },       1, "in" },            // in al, imm8
    { { 0xE5 },       1, "in" },
    { { 0xEC },       1, "in" },            // in al, dx
    { { 0xED },       1, "in" },
    { { 0xE6 },       1, "out" },
    { { 0xE7 },       1, "out" },
    { { 0xEE },       1, "out" },
    { { 0xEF },       1, "out" },
    { { 0xF6 },       1, "div" },           // div/idiv r/m8 (and mul, neg, ...)
    { { 0xF7 },       1, "div" },           // div/idiv r/m32
    { { 0xCC },       1, "int3" },
    { { 0xCD },       1, "int" },           // int imm8 to a kernel-only gate
    { { 0x0F, 0x0B }, 2, "ud2" },           // "undefined instruction" on purpose
    { { 0x0F, 0x31 }, 2, "rdtsc" },         // read the cycle counter
    { { 0x0F, 0x20 }, 2, "mov from cr" },   // read a control register
    { { 0x0F, 0x22 }, 2, "mov to cr" },     // write a control register
    { { 0x0F, 0x01 }, 2, "lgdt/lidt" },     // load descriptor tables
};
#define KNOWN_COUNT (sizeof(known) / sizeof(known[0]))

// Name the instruction at the faulting EIP, or 0. Only reads the
// program's own, mapped memory (user_ptr_bounds), so a wild EIP is safe.
static const char *instruction_at(const registers_t *regs) {
    if (!def_user_range_ok(regs->eip, 2))
        return 0;
    const uint8_t *code = (const uint8_t *)regs->eip;

    for (unsigned i = 0; i < KNOWN_COUNT; i++) {
        if (code[0] != known[i].bytes[0])
            continue;
        if (known[i].length == 1 || code[1] == known[i].bytes[1])
            return known[i].name;
    }
    return 0;
}

/* ---------- Recording ---------- */

void cpu_fault_record(const registers_t *regs) {
    process_t *self = scheduler_get_current_process();
    uint8_t vector = regs->int_no;

    cpu_fault_t *f = &log_ring[log_head];
    memset(f, 0, sizeof(*f));
    f->pid    = self ? self->pid : 0;
    strncpy(f->name, self ? self->name : "?", PROCESS_NAME_LEN - 1);
    f->vector = vector;
    f->eip    = regs->eip;
    f->instruction = instruction_at(regs);
    f->tick   = timer_ticks;

    log_head = (log_head + 1) % CPU_FAULT_LOG_SIZE;
    if (log_count < CPU_FAULT_LOG_SIZE)
        log_count++;
    if (vector < ISR_EXCEPTIONS)
        kills[vector]++;
    total++;

    char name[PROCESS_NAME_LEN];
    def_safe_text(f->name, name, PROCESS_NAME_LEN - 1);   // ui defence safe_text

    //   [CPU FAULT] #13 General Protection Fault: PID 6 (cli) killed
    //     EIP 0x40000000 cli: privileged instruction, port or segment
    ui_color(UI_BAD);
    ui_printf("\n[CPU FAULT] #%u %s: PID %u (%s) killed\n",
              vector, isr_exception_name(vector), f->pid, name);
    ui_color(UI_DIM);
    ui_printf("  EIP 0x%08x %s: %s\n", f->eip,
              f->instruction ? f->instruction : "", cpu_fault_hint(vector));
    ui_reset();
}

uint32_t cpu_fault_kills(uint8_t vector) {
    return vector < ISR_EXCEPTIONS ? kills[vector] : 0;
}

uint32_t cpu_fault_total(void) {
    return total;
}

int cpu_fault_log_count(void) {
    return log_count;
}

const cpu_fault_t *cpu_fault_log_get(int index) {
    if (index < 0 || index >= log_count)
        return 0;
    return &log_ring[(log_head - 1 - index + CPU_FAULT_LOG_SIZE) % CPU_FAULT_LOG_SIZE];
}

/* ---------- The handler ---------- */

static void fault_handler(registers_t *regs) {
    // DEFENCE fault_isolation: a fault in a ring 3 program is that
    // program's problem. End it, keep the OS running.
    if (def_fault_isolation_survivable(regs)) {
        cpu_fault_record(regs);

        // DEFENCE tsc_restrict: a ring 3 rdtsc faults on purpose; say so.
        def_tsc_restrict_note(regs->int_no, cpu_fault_log_get(0)->instruction);

        process_exit();             // never returns: the reaper frees it
    }

    // A fault in the kernel itself (or a broken machine): stop.
    isr_panic(regs);
}

void cpu_faults_init(void) {
    for (int vector = 0; vector < ISR_EXCEPTIONS; vector++)
        if (vector != PAGE_FAULT)
            register_interrupt_handler(vector, fault_handler);
}
