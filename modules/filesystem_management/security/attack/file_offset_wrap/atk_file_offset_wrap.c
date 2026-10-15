#include "fs_security.h"
#include "proc_security.h"
#include "vga.h"

static int atk_file_offset_wrap_check(void) {
    return !fs_security_file_range_valid(0xFFFFFFF0u, 32, 0xFFFFFFFFu) &&
           !fs_security_file_range_valid(16384, 1, 16384) &&
           fs_security_file_range_valid(16383, 1, 16384);
}

static int atk_file_offset_wrap_simulate(void) {
    vga_print_str("  checks wrapping offsets and writes beyond the RAM-disk limit\n");
    vga_print_str("  file data is not read or modified\n");
    return atk_file_offset_wrap_check();
}

static int atk_file_offset_wrap_launch(void) {
    vga_print_str("  probing the live RAM-disk range policy\n");
    return atk_file_offset_wrap_check();
}

static const attack_t atk = {
    "file_offset_wrap",
    "overflowing file offsets can bypass the RAM-disk size limit",
    INV_CONSISTENCY,
    SEV_HIGH,
    atk_file_offset_wrap_simulate,
    atk_file_offset_wrap_launch,
    0,
    "file_range"
};

void atk_file_offset_wrap_register(void) {
    proc_security_register_attack(&atk);
}
