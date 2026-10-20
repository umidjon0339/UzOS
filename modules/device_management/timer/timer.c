/*
 * timer.c - Program the PIT and count ticks.
 *
 * Every tick: count it, let the security lab's watchdog look at the
 * time, charge the tick to whoever had the CPU, and let the scheduler
 * switch tasks.
 *
 * Uses: register_interrupt_handler (arch/x86/isr), pic_unmask (pic/),
 *       scheduler_tick (process_management), sec_watchdog_tick
 *       (security_lab), cpu_account_tick (cpu_management), and the
 *       defence timer_frequency.
 */

#include "timer.h"
#include "isr.h"
#include "io.h"
#include "pic.h"
#include "scheduler.h"
#include "security_lab.h"
#include "cpu_account.h"
#include "def_timer_frequency.h"

#define TIMER_IRQ     0
#define PIT_CHANNEL0  0x40          // data port of counter 0
#define PIT_COMMAND   0x43
#define PIT_MODE3     0x36          // counter 0, low+high byte, square wave

volatile uint32_t timer_ticks;
static uint32_t frequency;

static void timer_callback(registers_t *regs) {
    timer_ticks++;
    sec_watchdog_tick();
    cpu_account_tick(regs);         // before the switch: charge who WAS running
    scheduler_tick();               // may switch to another task
}

void init_timer(uint32_t hz) {
    // DEFENCE timer_frequency: 0 would divide by zero, and the divisor
    // must fit the PIT's 16-bit counter.
    if (!def_timer_frequency_ok(hz))
        return;

    register_interrupt_handler(32 + TIMER_IRQ, timer_callback);

    uint32_t divisor = PIT_INPUT_HZ / hz;
    outb(PIT_COMMAND, PIT_MODE3);
    outb(PIT_CHANNEL0, divisor & 0xFF);         // low byte first
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);  // then high byte

    frequency = hz;
    pic_unmask(TIMER_IRQ);
}

uint32_t timer_frequency(void) {
    return frequency;
}

uint32_t timer_uptime_ms(void) {
    if (!frequency)
        return 0;
    return (uint64_t)timer_ticks * 1000 / frequency;   // 64-bit: no overflow
}

void timer_wait(uint32_t ticks) {
    uint32_t start = timer_ticks;
    while (timer_ticks - start < ticks)     // subtraction stays right after wrap-around
        asm volatile("hlt");
}
