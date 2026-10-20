/*
 * isr.c - Route every interrupt to the right C handler.
 *
 *   vectors 32-47 -> IRQ: acknowledge the PIC, then run the driver
 *   any vector with a registered handler -> run it
 *   unhandled exception (0-31) -> print registers and halt (isr_panic)
 *
 * cpu_management registers a handler for every CPU exception, so in
 * practice isr_panic() runs only when that handler gives up: a fault in
 * kernel code, or one no process can be blamed for.
 */

#include "isr.h"
#include "io.h"
#include "pic.h"
#include "vga.h"

static isr_t handlers[256];
static uint32_t irq_counts[ISR_IRQS];
static uint32_t spurious_count;

static const char *const exception_names[ISR_EXCEPTIONS] = {
    "Division By Zero",             //  0
    "Debug",                        //  1
    "Non Maskable Interrupt",       //  2
    "Breakpoint",                   //  3
    "Overflow",                     //  4
    "BOUND Range Exceeded",         //  5
    "Invalid Opcode",               //  6
    "No Coprocessor",               //  7
    "Double Fault",                 //  8
    "Coprocessor Segment Overrun",  //  9
    "Bad TSS",                      // 10
    "Segment Not Present",          // 11
    "Stack Fault",                  // 12
    "General Protection Fault",     // 13
    "Page Fault",                   // 14
    "Reserved",                     // 15
    "x87 Floating-Point Fault",     // 16
    "Alignment Check",              // 17
    "Machine Check",                // 18
    "SIMD Floating-Point Fault",    // 19
    "Virtualization Exception",     // 20
    "Control Protection Exception", // 21
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", // 22-27
    "Hypervisor Injection",         // 28
    "VMM Communication",            // 29
    "Security Exception",           // 30
    "Reserved"                      // 31
};

void register_interrupt_handler(uint8_t n, isr_t handler) {
    handlers[n] = handler;
}

int isr_has_handler(uint8_t n) {
    return handlers[n] != 0;
}

uint32_t isr_irq_count(uint8_t irq) {
    return irq < ISR_IRQS ? irq_counts[irq] : 0;
}

uint32_t isr_spurious_count(void) {
    return spurious_count;
}

const char *isr_exception_name(uint8_t n) {
    return n < ISR_EXCEPTIONS ? exception_names[n] : "?";
}

/* ---------- Exceptions ---------- */

static void print_reg(const char *name, uint32_t value) {
    vga_print_str(name);
    vga_print_hex(value);
}

// Show the CPU state and stop.
void isr_panic(registers_t *regs) {
    uint32_t cr2;   // page fault address
    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    vga_fullscreen(0);              // a full-screen program must not hide this
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_print_str("\n[EXCEPTION] ");
    vga_print_str(isr_exception_name(regs->int_no));
    vga_print_str(" (#");
    vga_print_int(regs->int_no);
    vga_print_str(")\n");

    print_reg(" EIP=", regs->eip);
    print_reg(" CS=", regs->cs);
    print_reg(" EFLAGS=", regs->eflags);
    print_reg(" ERR=", regs->err_code);
    vga_print_str("\n");
    print_reg(" EAX=", regs->eax);
    print_reg(" EBX=", regs->ebx);
    print_reg(" ECX=", regs->ecx);
    print_reg(" EDX=", regs->edx);
    vga_print_str("\n");
    print_reg(" ESI=", regs->esi);
    print_reg(" EDI=", regs->edi);
    print_reg(" EBP=", regs->ebp);
    print_reg(" CR2=", cr2);
    vga_print_str("\n");

    if ((regs->cs & 3) == 3) {      // came from ring 3: CPU also pushed ESP/SS
        print_reg(" USER ESP=", regs->useresp);
        print_reg(" SS=", regs->ss);
        vga_print_str("\n");
    }

    vga_print_str("System Halted!\n");
    asm volatile("cli");
    while (1) asm volatile("hlt");
}

/* ---------- Hardware IRQs ---------- */

// Read a PIC's In-Service Register: which IRQs it is currently serving.
static uint8_t pic_in_service(uint16_t command_port) {
    outb(command_port, 0x0B);
    return inb(command_port);
}

// A "spurious" IRQ 7/15 is a request that vanished before the CPU took it.
// The PIC's in-service bit is then clear, and that PIC must get no EOI.
static int is_spurious(uint8_t irq) {
    if (irq == 7 && !(pic_in_service(PIC1_COMMAND) & 0x80))
        return 1;
    if (irq == 15 && !(pic_in_service(PIC2_COMMAND) & 0x80)) {
        outb(PIC1_COMMAND, PIC_EOI);    // master did see the cascade IRQ 2
        return 1;
    }
    return 0;
}

static void handle_irq(registers_t *regs) {
    uint8_t irq = regs->int_no - IRQ_BASE;

    if (is_spurious(irq)) {
        spurious_count++;
        return;
    }
    irq_counts[irq]++;

    // EOI first: the timer handler may switch tasks and not return soon.
    pic_send_eoi(irq);

    if (handlers[regs->int_no])
        handlers[regs->int_no](regs);
}

/* ---------- Entry from isr_stubs.s ---------- */

void isr_handler(registers_t *regs) {
    uint32_t n = regs->int_no;

    if (n >= IRQ_BASE && n < IRQ_BASE + ISR_IRQS) {
        handle_irq(regs);
    } else if (handlers[n]) {
        handlers[n](regs);          // e.g. page fault (14), syscall (128)
    } else if (n < ISR_EXCEPTIONS) {
        isr_panic(regs);
    }
}
