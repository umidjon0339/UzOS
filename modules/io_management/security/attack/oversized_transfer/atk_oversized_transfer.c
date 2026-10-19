#include "io_security.h"
#include "io_syscalls.h"
#include "proc_security.h"
#include "vga.h"

static int atk_oversized_transfer_simulate(void) {
    static const char sample[] = "safe";
    int checked = 0;

    if (io_buffer_valid(sample, sizeof(sample))) checked++;
    if (!io_buffer_valid(0, 1)) checked++;
    if (!io_buffer_valid(sample, IO_MAX_TRANSFER + 1)) checked++;
    if (!io_buffer_valid((const void *)0xFFFFFFF0u, 32)) checked++;

    vga_print_str("  checked null, oversized, wrapped, and valid buffer ranges\n");
    vga_print_str("  no buffer is dereferenced\n");
    return checked;
}

static int atk_oversized_transfer_launch(void) {
    int result = sys_write(IO_STDOUT, 0, IO_MAX_TRANSFER + 1);

    vga_print_str("  oversized write returned ");
    vga_print_int(result);
    vga_print_str(" (expected -1)\n");
    return result == -1;
}

static const attack_t atk = {
    "oversized_transfer",
    "unvalidated I/O buffer sizes can cause oversized memory access",
    INV_TRUST,
    SEV_HIGH,
    atk_oversized_transfer_simulate,
    atk_oversized_transfer_launch,
    0,
    "transfer_bounds"
};

void atk_oversized_transfer_register(void) {
    proc_security_register_attack(&atk);
}
