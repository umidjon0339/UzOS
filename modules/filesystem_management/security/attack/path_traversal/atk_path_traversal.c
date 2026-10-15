#include "fs_security.h"
#include "proc_security.h"
#include "vga.h"

static int atk_path_traversal_check(void) {
    static const char *const paths[] = {
        "report", "/report", "..", "../secret", "dir/secret"
    };
    static const int expected[] = { 1, 1, 0, 0, 0 };
    int blocked = 0;

    for (unsigned int i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
        if (fs_security_check_path(paths[i]) == expected[i]) blocked++;
    return blocked;
}

static int atk_path_traversal_simulate(void) {
    vga_print_str("  checks parent traversal and unsupported nested paths\n");
    vga_print_str("  no filesystem operation is performed\n");
    return atk_path_traversal_check();
}

static int atk_path_traversal_launch(void) {
    vga_print_str("  probing the live filesystem path policy\n");
    return atk_path_traversal_check();
}

static const attack_t atk = {
    "path_traversal",
    "crafted paths can escape a directory or bypass simple-name lookup",
    INV_TRUST,
    SEV_HIGH,
    atk_path_traversal_simulate,
    atk_path_traversal_launch,
    0,
    "path_component"
};

void atk_path_traversal_register(void) {
    proc_security_register_attack(&atk);
}
