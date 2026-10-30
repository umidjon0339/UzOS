/*
 * atk_argv_overflow.c - ATTACK argv_overflow: drown execve in arguments.
 *
 * Breaks:     resource - execve copies argv/envp into the kernel and onto
 *             the new program's stack
 * Stopped by: arg_limits (defence/arg_limits)
 *
 * execve copies every argument string twice: into a kernel buffer, then
 * onto the new stack. With no limit, a million arguments (or one huge
 * string) overflow the buffer or fill the stack before main() runs - or
 * push the stack into other memory (stack clash). An argument pointer
 * that points into the KERNEL makes execve copy kernel memory into the
 * new program, where it can be read.
 *
 * simulate(): show the sizes.
 * launch():   hand arg_limits what execve would be handed: 100
 *             arguments, a 3000-byte argument, and (as from ring 3) an
 *             argument pointing at the kernel's code. findings = accepted.
 *             (`proccalls exec` sends the same from a real ring 3 program.)
 * cleanup():  nothing.
 *
 * Uses: def_args_ok (its defence), memset, ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/process_management/security/argv_overflow/defence/arg_limits/def_arg_limits.h"  // def_args_ok
#include "lib/string/string.h"                                    // memset
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define MANY 100
#define LONG 3000

static char *many[MANY + 1];
static char long_arg[LONG + 1];

static int simulate(void) {
    ui_printf("  execve limits: %d strings, %d bytes (argv + envp)\n", ARG_MAX_STRINGS, ARG_MAX_BYTES);
    ui_printf("  the attack: %d arguments / one of %d bytes / one in the kernel\n", MANY, LONG);
    return 3;
}

static int verdict(const char *what, int accepted) {
    ui_printf("  %-34s ", what);
    ui_color(accepted ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", accepted ? "accepted!" : "refused (E2BIG / EFAULT)");
    ui_reset();
    return accepted;
}

static int launch(void) {
    int bad = 0;
    uint32_t count, bytes;

    for (int i = 0; i < MANY; i++)
        many[i] = "x";
    many[MANY] = 0;
    count = bytes = 0;
    bad += verdict("execve(100 arguments)", def_args_ok(many, 0, &count, &bytes));

    memset(long_arg, 'A', LONG);
    long_arg[LONG] = '\0';
    char *one_long[] = { long_arg, 0 };
    count = bytes = 0;
    bad += verdict("execve(one 3000-byte argument)", def_args_ok(one_long, 0, &count, &bytes));

    char *into_kernel[] = { "prog", (char *)0x00100000, 0 };
    count = bytes = 0;
    bad += verdict("execve(argv[1] = kernel code)", def_args_ok(into_kernel, 1, &count, &bytes));
    return bad;
}

static int cleanup(void) { return 0; }

static const attack_t attack = {
    "argv_overflow",
    "execve with huge argv / envp, or argument pointers into the kernel",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "arg_limits"
};

void atk_argv_overflow_register(void) { registry_add_attack(&attack); }
