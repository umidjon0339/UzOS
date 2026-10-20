/*
 * atk_bad_filename.c - ATTACK bad_filename: create names that break paths.
 *
 * Breaks:     trust - names from outside are checked before use
 * Stopped by: filename_check (security/defence/filename_check)
 *
 * simulate(): list the names and what each would break.
 * launch():   really try to create each one in /, and look up a path
 *             with a 300-character component. Anything that got created
 *             by mistake is deleted again.
 *
 * Uses: ramdisk_create / ramdisk_delete (core/ramdisk), vfs_lookup (core/vfs).
 */

#include "security_lab.h"
#include "ramdisk.h"
#include "string.h"
#include "vga.h"

static const struct { const char *name; const char *problem; } bad[] = {
    { "..",      "can never be opened: '..' means parent" },
    { ".",       "can never be opened: '.' means here" },
    { "a/b",     "'/' splits paths: unreachable" },
    { "my file", "the shell splits at spaces: cannot be typed" },
    { "",        "an empty name" },
};
#define BAD_COUNT (sizeof(bad) / sizeof(bad[0]))
#define LONG_LEN  300

static int simulate(void) {
    for (unsigned i = 0; i < BAD_COUNT; i++) {
        vga_print_str("  \"");
        vga_print_str(bad[i].name);
        vga_print_str("\" - ");
        vga_print_str(bad[i].problem);
        vga_print_str("\n");
    }
    vga_print_str("  a 300-char name - overflows the 128-byte name buffer\n");
    return BAD_COUNT + 1;
}

static int launch(void) {
    uint32_t denied = sec_total_denied();

    for (unsigned i = 0; i < BAD_COUNT; i++) {
        vfs_node_t *made = ramdisk_create(fs_root, bad[i].name, FS_FILE);
        vga_print_str("  create \"");
        vga_print_str(bad[i].name);
        vga_print_str(made ? "\" -> CREATED\n" : "\" -> refused\n");
        if (made)
            ramdisk_delete(fs_root, bad[i].name);
    }

    // A path whose one component is longer than any name buffer.
    static char long_path[LONG_LEN + 1];
    memset(long_path, 'x', LONG_LEN);
    long_path[LONG_LEN] = '\0';
    vfs_node_t *found = vfs_lookup(fs_root, long_path);
    vga_print_str(found ? "  lookup of a 300-char name -> FOUND?\n"
                        : "  lookup of a 300-char name -> refused, nothing copied\n");

    return sec_total_denied() - denied;
}

static const attack_t attack = {
    "bad_filename",
    "names like '..', 'a/b' or 300 characters break paths or overflow buffers",
    INV_TRUST, SEV_HIGH,
    simulate, launch, 0,        // launch deletes anything it managed to create
    "filename_check"
};

void atk_bad_filename_register(void) {
    proc_security_register_attack(&attack);
}
