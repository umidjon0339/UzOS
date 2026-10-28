/*
 * cmd_turbostat.c - `turbostat`: clock, busy time and idle per interval.
 *
 *   Core  CPU  Avg_MHz  Busy%  Bzy_MHz  TSC_MHz     IRQ    POLL%     C1%
 *      -    -       80   3.00        -     2686     102     0.00   97.00
 *      0    0       80   3.00        -     2686     102     0.00   97.00
 *
 *   Avg_MHz   cycles really spent working per second = Busy% x TSC_MHz
 *   Busy%     time not in the idle task
 *   Bzy_MHz   the clock WHILE busy: needs the APERF/MPERF counters of a
 *             real CPU (QEMU has none): "-"
 *   TSC_MHz   the cycle counter's rate (core/tsc)
 *   IRQ       interrupts in the interval
 *   POLL% C1% idle time by idle state (core/power: spin or hlt)
 *
 *   turbostat              every 5 s until Ctrl+C (Linux's default)
 *   turbostat -i 1 -n 3    every second, three times
 *
 * Linux's turbostat reads model-specific registers (MSRs) and so needs
 * root; UzOS computes everything from its own counters.
 *
 * Uses: cpu_account_totals / _interrupts (core/accounting), tsc_mhz
 *       (core/tsc), power_idle_state (core/power), sys_sleep_interruptible
 *       (process_management), ui_* (ui), arg_next / atoi / strcmp (lib).
 */

#include "modules/cpu_management/commands/turbostat/cmd_turbostat.h"   // this command
#include "modules/cpu_management/core/accounting/cpu_account.h"        // cpu_account_*
#include "modules/cpu_management/core/tsc/tsc.h"                       // tsc_mhz
#include "modules/cpu_management/core/power/power.h"                   // power_idle_state
#include "modules/process_management/syscalls/sleep/sys_sleep.h"       // sys_sleep_interruptible
#include "modules/ui/core/style/ui.h"                                  // ui_*
#include "lib/args/args.h"                                             // arg_next
#include "lib/stdlib/stdlib.h"                                         // atoi
#include "lib/string/string.h"                                         // strcmp, strlen
#include "lib/format/format.h"                                         // format

#define DEFAULT_INTERVAL 5

static void two_decimals(uint32_t hundredths, int width) {
    // width counts the whole "97.00"
    char text[16];
    format(text, sizeof(text), "%u.%02u", hundredths / 100, hundredths % 100);
    for (int i = strlen(text); i < width; i++)
        vga_print_str(" ");
    vga_print_str(text);
}

static void row(const char *core, const char *cpu, uint32_t busy_h, uint32_t irqs, int polling) {
    uint32_t idle_h = 10000 - busy_h;
    ui_printf("%5s %4s %8u ", core, cpu, (uint32_t)((uint64_t)tsc_mhz() * busy_h / 10000));
    two_decimals(busy_h, 6);
    ui_printf(" %8s %8u %7u ", "-", tsc_mhz(), irqs);
    two_decimals(polling ? idle_h : 0, 8);
    vga_print_str(" ");
    two_decimals(polling ? 0 : idle_h, 7);
    vga_print_str("\n");
}

void cmd_turbostat(const char *args) {
    char word[16];
    int interval = DEFAULT_INTERVAL, count = -1;

    for (args = arg_next(args, word, sizeof(word)); word[0];
         args = arg_next(args, word, sizeof(word))) {
        if (strcmp(word, "-i") == 0 || strcmp(word, "--interval") == 0) {
            args = arg_next(args, word, sizeof(word));
            interval = atoi(word);
        } else if (strcmp(word, "-n") == 0 || strcmp(word, "--num_iterations") == 0) {
            args = arg_next(args, word, sizeof(word));
            count = atoi(word);
        } else {
            ui_usage("turbostat [-i seconds] [-n count]");
            return;
        }
    }
    if (interval < 1)
        interval = 1;

    cpu_ticks_t before, after;
    uint32_t irq_before = cpu_account_interrupts();
    cpu_account_totals(&before);

    for (int done = 0; count < 0 || done < count; done++) {
        if (!sys_sleep_interruptible(interval * 1000))
            break;                                  // Ctrl+C
        cpu_account_totals(&after);
        uint32_t irq_after = cpu_account_interrupts();

        uint32_t busy = (after.user + after.nice + after.system) -
                        (before.user + before.nice + before.system);
        uint32_t all = busy + after.idle - before.idle;
        uint32_t busy_h = all ? (uint32_t)((uint64_t)busy * 10000 / all) : 0;
        int polling = strcmp(power_idle_state(), "POLL") == 0;

        ui_color(UI_TITLE);
        vga_print_str(" Core  CPU  Avg_MHz  Busy%  Bzy_MHz  TSC_MHz     IRQ    POLL%     C1%\n");
        ui_reset();
        row("-", "-", busy_h, irq_after - irq_before, polling);
        row("0", "0", busy_h, irq_after - irq_before, polling);

        before = after;
        irq_before = irq_after;
    }
}
