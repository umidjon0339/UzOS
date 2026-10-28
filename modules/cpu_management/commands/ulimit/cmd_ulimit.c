/*
 * cmd_ulimit.c - `ulimit`: the shell's own resource limits.
 *
 *   ulimit             the file size limit (bash shows this one alone)
 *   ulimit -a          every limit UzOS uses, with its letter
 *   ulimit -t          one limit (soft): -t cpu  -f fsize  -d data
 *                      -s stack  -u nproc  -n nofile  -e nice  -r rtprio
 *   ulimit -t 2        set it (soft AND hard, like bash)
 *   ulimit -S -t 2     only the soft limit;  -H: only the hard limit
 *   ulimit -t unlimited
 *
 * Like in bash it is a SHELL setting: it changes the limits of the
 * shell's process (kernel_main), and every command started from now on
 * inherits them (core/task). Raising a hard limit needs root
 * (CAP_RESOURCE); anyone may lower one, and then cannot raise it back.
 *
 * Uses: rlimit_* (core/rlimit), the current process (process_management),
 *       ui_* (ui), arg_next / strtoul / strcmp (lib).
 */

#include "modules/cpu_management/commands/ulimit/cmd_ulimit.h"      // this command
#include "modules/cpu_management/core/rlimit/rlimit.h"              // rlimit_*
#include "modules/process_management/core/scheduler/scheduler.h"    // the current process
#include "modules/ui/core/style/ui.h"                               // ui_*
#include "lib/args/args.h"                                          // arg_next
#include "lib/stdlib/stdlib.h"                                      // strtoul
#include "lib/string/string.h"                                      // strcmp, strcpy
#include "lib/format/format.h"                                      // format

// The limit whose ulimit letter is 'c', or -1.
static int resource_of(char c) {
    for (int r = 0; r < RLIMIT_COUNT; r++)
        if (rlimit_supported(r) && rlimit_option(r) == c)
            return r;
    return -1;
}

static void print_value(uint32_t value) {
    if (value == RLIM_INFINITY)
        vga_print_str("unlimited");
    else
        ui_printf("%u", value);
}

static void print_all(const process_t *self, int hard) {
    for (int r = 0; r < RLIMIT_COUNT; r++) {
        if (!rlimit_supported(r))
            continue;
        char label[48];
        format(label, sizeof(label), "%s (%s, -%c)", rlimit_name(r), rlimit_unit(r),
               rlimit_option(r));
        ui_color(UI_LABEL);
        ui_printf("%-40s ", label);
        ui_reset();
        print_value(hard ? self->rlim_max[r] : self->rlim_cur[r]);
        vga_print_str("\n");
    }
}

void cmd_ulimit(const char *args) {
    process_t *self = scheduler_get_current_process();
    char word[24];
    int soft = 1, hard = 1, explicit_side = 0, resource = RLIMIT_FSIZE;
    const char *value = 0;
    char value_text[24];

    for (args = arg_next(args, word, sizeof(word)); word[0];
         args = arg_next(args, word, sizeof(word))) {
        if (strcmp(word, "-a") == 0) {
            print_all(self, explicit_side && hard && !soft);
            return;
        } else if (strcmp(word, "-S") == 0 || strcmp(word, "-H") == 0) {
            soft = word[1] == 'S';
            hard = word[1] == 'H';
            explicit_side = 1;
        } else if (word[0] == '-' && word[1] && !word[2] && resource_of(word[1]) >= 0) {
            resource = resource_of(word[1]);
        } else if (!value) {
            strcpy(value_text, word);
            value = value_text;
        } else {
            ui_usage("ulimit [-SH] [-a | -tfdsuner [value | unlimited]]");
            return;
        }
    }

    if (!value) {                               // show one limit
        if (!explicit_side)
            hard = 0;                           // bash shows the soft one
        print_value(hard ? self->rlim_max[resource] : self->rlim_cur[resource]);
        vga_print_str("\n");
        return;
    }

    char *end;
    uint32_t number = strcmp(value, "unlimited") == 0 ? RLIM_INFINITY
                                                      : strtoul(value, &end, 10);
    if (number != RLIM_INFINITY && (*end || end == value)) {
        ui_error("ulimit", "not a number (or 'unlimited')");
        return;
    }

    rlimit_t limit = { self->rlim_cur[resource], self->rlim_max[resource] };
    if (soft) limit.cur = number;
    if (hard) limit.max = number;
    if (!rlimit_set(self, self, resource, &limit))
        ui_error("ulimit", "refused: soft above hard, raise needs root, or not possible");
}
