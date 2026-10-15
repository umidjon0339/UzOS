#include "timer.h"
#include "isr.h"
#include "io.h"
#include "vga.h"
#include "scheduler.h"
#include "proc_security.h"
#include "device_security.h"

volatile uint32_t timer_ticks = 0;

static void timer_callback(registers_t *regs) {
    (void)regs;
    timer_ticks++;

    /* Let the security lab's watchdog notice an attack that overran
     * its time budget. */
    sec_watchdog_tick();

    scheduler_tick();
}


void init_timer(uint32_t frequency) {
    if (!device_timer_frequency_valid(frequency)) {
        sec_log("timer_frequency", "invalid PIT frequency refused");
        vga_print_str("[TIMER ERROR] Invalid PIT frequency; timer unchanged.\n");
        return;
    }

    // Register our timer callback
    register_interrupt_handler(32, timer_callback); // IRQ0 is vector 32

    // The value we send to the PIT is the value to divide it's input clock
    // (1193180 Hz) by, to get our required frequency.
    uint32_t divisor = PIT_INPUT_HZ / frequency;

    // Send the command byte
    outb(0x43, 0x36); // Command port

    // Divisor has to be sent byte-wise, so split here into upper/lower bytes.
    uint8_t l = (uint8_t)(divisor & 0xFF);
    uint8_t h = (uint8_t)( (divisor>>8) & 0xFF );

    // Send the frequency divisor
    outb(0x40, l);
    outb(0x40, h);
}

void timer_wait(uint32_t ticks) {
    uint32_t end_ticks = timer_ticks + ticks;
    while(timer_ticks < end_ticks) {
        asm volatile("hlt"); // halt until next interrupt
    }
}
