/*
 * atk_dir_cycle.c - ATTACK dir_cycle: move a directory inside itself.
 *
 * Breaks:     consistency - the directory tree stays a tree
 * Stopped by: no_cycle (security/defence/no_cycle)
 *
 * "mv /a /a/b" would make a's parent one of its own children: the
 * subtree is cut off from / and parent-walking code loops forever.
 *
 * simulate(): show the loop it would make.
 * launch():   build /seclab_a/b, try to move seclab_a into b, and into
 *             itself under a new name, then remove both directories.
 *
 * Uses: sys_mkdir / sys_rename / sys_unlink (syscalls/).
 */

#include "registry.h"
#include "audit.h"
#include "sys_mkdir.h"
#include "sys_rename.h"
#include "sys_unlink.h"
#include "vga.h"

static int simulate(void) {
    vga_print_str("  before: / -> seclab_a -> b\n");
    vga_print_str("  mv seclab_a seclab_a/b: seclab_a's parent = b, b's parent = seclab_a\n");
    vga_print_str("  -> a loop cut off from /; pwd inside it never ends\n");
    return 1;
}

static void try_move(const char *what, const char *from, const char *to) {
    vga_print_str("  ");
    vga_print_str(what);
    vga_print_str(sys_rename(from, to) == 0 ? " -> MOVED\n" : " -> refused\n");
}

static int launch(void) {
    uint32_t denied = audit_total();

    if (sys_mkdir("/seclab_a") || sys_mkdir("/seclab_a/b")) {
        vga_print_str("  could not build the test directories\n");
        return 0;
    }
    try_move("mv seclab_a into seclab_a/b", "/seclab_a", "/seclab_a/b");
    try_move("mv seclab_a seclab_a/inner ", "/seclab_a", "/seclab_a/inner");

    sys_unlink("/seclab_a/b");
    sys_unlink("/seclab_a");
    return audit_total() - denied;
}

static const attack_t attack = {
    "dir_cycle",
    "move a directory into its own subdirectory, making a loop",
    INV_CONSISTENCY, SEV_HIGH,
    simulate, launch, 0,        // launch removes its test directories
    "no_cycle"
};

void atk_dir_cycle_register(void) {
    registry_add_attack(&attack);
}
