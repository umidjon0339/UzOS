#include "io_security.h"
#include "proc_security.h"
#include "vfs.h"

static int def_descriptor_bounds_selftest(void) {
    process_t empty = {0};
    vfs_node_t node = {0};

    if (io_fd_index_valid(-1)) return 0;
    if (!io_fd_index_valid(0)) return 0;
    if (!io_fd_index_valid(MAX_FDS - 1)) return 0;
    if (io_fd_index_valid(MAX_FDS)) return 0;
    if (io_file_descriptor_valid(&empty, IO_STDIN)) return 0;
    if (io_file_descriptor_valid(&empty, IO_FIRST_FILE_FD)) return 0;
    empty.fd_table[IO_FIRST_FILE_FD] = &node;
    if (!io_file_descriptor_valid(&empty, IO_FIRST_FILE_FD)) return 0;
    return 1;
}

static const defence_t def = {
    "descriptor_bounds",
    "validate descriptor indexes and require an open file before access",
    "invalid_descriptor",
    def_descriptor_bounds_selftest
};

void def_descriptor_bounds_register(void) {
    proc_security_register_defence(&def);
}
