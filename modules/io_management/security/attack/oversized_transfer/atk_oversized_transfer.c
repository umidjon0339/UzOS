/*
 * atk_oversized_transfer.c - ATTACK oversized_transfer: bad read/write buffers.
 *
 * Breaks:     trust - a buffer is checked before the kernel copies into it
 * Stopped by: transfer_bounds (security/defence/transfer_bounds)
 *
 * simulate(): list the bad buffers.
 * launch():   really call read/write with each one; a normal write must
 *             still work.
 *
 * Uses: sys_read / sys_write (syscalls/).
 */

#include "registry.h"
#include "audit.h"
#include "sys_read.h"
#include "sys_write.h"
#include "fd.h"
#include "vga.h"

static int simulate(void) {
    vga_print_str("  write(stdout, NULL, 5): copy from address 0\n");
    vga_print_str("  write(stdout, buf, 16 KiB + 1): too big\n");
    vga_print_str("  read(stdin, 0xFFFFFFF0, 32): wraps round to address 0x10\n");
    return 3;
}

static void report(const char *what, int result) {
    vga_print_str("  ");
    vga_print_str(what);
    vga_print_str(result < 0 ? " -> refused\n" : " -> ALLOWED\n");
}

static int launch(void) {
    uint32_t denied = audit_total();
    static char buf[16];

    report("write(1, NULL, 5)         ", sys_write(STDOUT, 0, 5));
    report("write(1, buf, 16 KiB + 1) ", sys_write(STDOUT, buf, IO_MAX_TRANSFER + 1));
    report("read(0, 0xFFFFFFF0, 32)   ", sys_read(STDIN, (void *)0xFFFFFFF0, 32));

    vga_print_str("  normal write: ");
    sys_write(STDOUT, "ok\n", 3);
    return audit_total() - denied;
}

static const attack_t attack = {
    "oversized_transfer",
    "read/write with a NULL, huge, or wrapping buffer",
    INV_TRUST, SEV_HIGH,
    simulate, launch, 0,        // nothing was copied
    "transfer_bounds"
};

void atk_oversized_transfer_register(void) {
    registry_add_attack(&attack);
}
