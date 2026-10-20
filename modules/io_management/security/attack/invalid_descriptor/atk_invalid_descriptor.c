/*
 * atk_invalid_descriptor.c - ATTACK invalid_descriptor: use made-up fds.
 *
 * Breaks:     trust - descriptor numbers from a program are checked
 * Stopped by: descriptor_bounds (security/defence/descriptor_bounds)
 *
 * simulate(): list the bad uses.
 * launch():   really call read/write/close with each one, including a
 *             write to a file opened read-only, then clean up.
 *
 * Uses: sys_open / sys_read / sys_write / sys_close (syscalls/),
 *       sys_unlink (filesystem_management).
 */

#include "security_lab.h"
#include "sys_open.h"
#include "sys_read.h"
#include "sys_write.h"
#include "sys_close.h"
#include "sys_unlink.h"
#include "fd.h"
#include "vga.h"

#define TEMP "/seclab_fd"

static int simulate(void) {
    vga_print_str("  read(-1), read(16): indexes outside the 16-entry table\n");
    vga_print_str("  read(9): a descriptor that was never opened\n");
    vga_print_str("  write(0), read(1): stdin is input only, stdout output only\n");
    vga_print_str("  write() to a file opened with O_READ\n");
    vga_print_str("  close(1): standard streams are not files\n");
    return 6;
}

static void report(const char *what, int result) {
    vga_print_str("  ");
    vga_print_str(what);
    vga_print_str(result < 0 ? " -> refused\n" : " -> ALLOWED\n");
}

static int launch(void) {
    uint32_t denied = sec_total_denied();
    char byte = 'x';

    report("read(-1)            ", sys_read(-1, &byte, 1));
    report("read(16)            ", sys_read(MAX_FDS, &byte, 1));
    report("read(9), never open ", sys_read(9, &byte, 1));
    report("write(stdin)        ", sys_write(STDIN, &byte, 1));
    report("read(stdout)        ", sys_read(STDOUT, &byte, 1));
    report("close(stdout)       ", sys_close(STDOUT));

    int fd = sys_open(TEMP, O_READ | O_CREATE);
    if (fd >= 0) {
        report("write to O_READ file", sys_write(fd, &byte, 1));
        sys_close(fd);
        sys_unlink(TEMP);
    }
    return sec_total_denied() - denied;
}

static const attack_t attack = {
    "invalid_descriptor",
    "use descriptor numbers that are out of range, closed, or the wrong kind",
    INV_TRUST, SEV_HIGH,
    simulate, launch, 0,        // the temporary file is deleted again
    "descriptor_bounds"
};

void atk_invalid_descriptor_register(void) {
    proc_security_register_attack(&attack);
}
