#include "io_security.h"
#include "io_syscalls.h"
#include "proc_security.h"
#include "vga.h"

static int atk_invalid_descriptor_simulate(void) {
    static const int probes[] = { -1, 0, MAX_FDS - 1, MAX_FDS };
    static const int expected[] = { 0, 1, 1, 0 };
    int checked = 0;

    for (unsigned int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++)
        if (io_fd_index_valid(probes[i]) == expected[i]) checked++;

    vga_print_str("  probed invalid and valid descriptor boundaries\n");
    vga_print_str("  no descriptor table was accessed\n");
    return checked;
}

static int atk_invalid_descriptor_launch(void) {
    int result = sys_read(-1, 0, 1);
    vga_print_str("  invalid read returned ");
    vga_print_int(result);
    vga_print_str(" (expected -1)\n");
    return result == -1;
}

static const attack_t atk = {
    "invalid_descriptor",
    "an invalid or closed descriptor can access unrelated process state",
    INV_TRUST,
    SEV_HIGH,
    atk_invalid_descriptor_simulate,
    atk_invalid_descriptor_launch,
    0,
    "descriptor_bounds"
};

void atk_invalid_descriptor_register(void) {
    proc_security_register_attack(&atk);
}
