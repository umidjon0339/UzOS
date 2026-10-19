#include "io_security.h"
#include "proc_security.h"

static int def_transfer_bounds_selftest(void) {
    static const char sample[] = "io";

    if (!io_buffer_valid(sample, sizeof(sample))) return 0;
    if (!io_buffer_valid(0, 0)) return 0;
    if (io_buffer_valid(0, 1)) return 0;
    if (io_buffer_valid(sample, IO_MAX_TRANSFER + 1)) return 0;
    if (io_buffer_valid((const void *)0xFFFFFFF0u, 32)) return 0;
    return 1;
}

static const defence_t def = {
    "transfer_bounds",
    "reject null, oversized, or wrapped I/O buffers",
    "oversized_transfer",
    def_transfer_bounds_selftest
};

void def_transfer_bounds_register(void) {
    proc_security_register_defence(&def);
}
