#include "fs_security.h"
#include "proc_security.h"

int fs_security_file_range_valid(uint32_t offset, uint32_t count,
                                 uint32_t limit) {
    return offset <= limit && count <= limit - offset;
}

static int def_file_range_selftest(void) {
    if (!fs_security_file_range_valid(0, 1, 1)) return 0;
    if (!fs_security_file_range_valid(10, 0, 10)) return 0;
    if (!fs_security_file_range_valid(9, 1, 10)) return 0;
    if (fs_security_file_range_valid(10, 1, 10)) return 0;
    if (fs_security_file_range_valid(0xFFFFFFF0u, 32, 0xFFFFFFFFu)) return 0;
    if (fs_security_file_range_valid(11, 0, 10)) return 0;
    return 1;
}

static const defence_t def = {
    "file_range",
    "validate file offsets and lengths without overflowing",
    "file_offset_wrap",
    def_file_range_selftest
};

void def_file_range_register(void) {
    proc_security_register_defence(&def);
}
