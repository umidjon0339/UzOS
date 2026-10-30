/*
 * atk_file_offset_wrap.c - ATTACK file_offset_wrap: write past a file.
 *
 * Breaks:     consistency - a write stays inside its own file
 * Stopped by: file_range (security/defence/file_range)
 *
 * The check "offset + size <= limit" wraps in 32 bits: 0xFFFFFFF0 + 0x20
 * is 0x10. The copy would then land 4 GiB away from the file's buffer.
 *
 * simulate(): show what the naive check computes.
 * launch():   make a temporary file, try the wrapping write and a write
 *             one byte past the size limit, then a normal write; delete it.
 *
 * Uses: ramdisk_create / ramdisk_delete (core/ramdisk), vfs_write (core/vfs).
 */

#include "modules/security_management/core/registry/registry.h"  // attack_t, registry_add_attack
#include "modules/security_management/core/audit/audit.h"        // audit_total
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"  // ramdisk_create, RAMDISK_MAX_FILE_SIZE, ramdisk_delete
#include "modules/device_management/vga/vga.h"                   // vga_print_str, vga_print_hex

#define TEMP_NAME "seclab_wrap"

static int simulate(void) {
    uint32_t offset = 0xFFFFFFF0, size = 0x20;
    vga_print_str("  naive: offset 0xFFFFFFF0 + size 0x20 = ");
    vga_print_hex(offset + size);
    vga_print_str(" <= 64 KiB limit -> passes\n");
    return 1;
}

static void try_write(vfs_node_t *f, const char *what, uint32_t offset, uint32_t size) {
    static uint8_t bytes[4] = { 'u', 'z', 'o', 's' };
    uint32_t written = vfs_write(f, offset, size, bytes);
    vga_print_str("  ");
    vga_print_str(what);
    vga_print_str(written ? " -> WRITTEN\n" : " -> refused\n");
}

static int launch(void) {
    vfs_node_t *f = ramdisk_create(fs_root, TEMP_NAME, FS_FILE);
    if (!f)
        return 0;
    uint32_t denied = audit_total();

    try_write(f, "offset 0xFFFFFFF0, size 0x20   ", 0xFFFFFFF0, 0x20);
    try_write(f, "offset 64 KiB - 1, size 2      ", RAMDISK_MAX_FILE_SIZE - 1, 2);
    try_write(f, "offset 0, size 4 (normal write)", 0, 4);

    ramdisk_delete(fs_root, TEMP_NAME);
    return audit_total() - denied;
}

static const attack_t attack = {
    "file_offset_wrap",
    "a huge file offset wraps around and writes outside the file",
    INV_CONSISTENCY, SEV_HIGH,
    simulate, launch, 0,        // the temporary file is deleted again
    "file_range"
};

void atk_file_offset_wrap_register(void) {
    registry_add_attack(&attack);
}
