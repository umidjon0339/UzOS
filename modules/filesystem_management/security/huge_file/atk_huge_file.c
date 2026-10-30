/*
 * atk_huge_file.c - ATTACK huge_file: one call, all the kernel's memory.
 *
 * Breaks:     resource - the RAM disk lives in the kernel heap
 * Stopped by: size_cap (defence/size_cap)
 *
 * write() grows a file a few bytes at a time. truncate(file, 2 GiB)
 * asks for the whole size in one call: on a RAM disk that is one
 * kmalloc of the entire kernel heap. Every other file, process stack
 * and buffer then fails to allocate.
 *
 * simulate(): the sizes.
 * launch():   create /seclab_huge and ask truncate() for 2 GiB, then for
 *             64 KiB + 1 (the real calls; the shell is root).
 *             findings = sizes accepted.
 * cleanup():  delete /seclab_huge.
 *
 * Uses: sys_truncate (syscalls/truncate), ramdisk_create / ramdisk_delete
 *       (core/ramdisk), fs_root (core/vfs), RAMDISK_MAX_FILE_SIZE, ui.
 */

#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/filesystem_management/syscalls/truncate/sys_truncate.h"  // sys_truncate
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"   // ramdisk_create, RAMDISK_MAX_FILE_SIZE
#include "modules/filesystem_management/core/vfs/vfs.h"           // fs_root, vfs_finddir
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color

#define FILE_NAME "seclab_huge"

static int simulate(void) {
    ui_printf("  truncate(file, 2 GiB): one kmalloc of the whole kernel heap\n");
    ui_printf("  a file may be %u bytes at most\n", RAMDISK_MAX_FILE_SIZE);
    return 2;
}

static int try_size(uint32_t size, const char *label) {
    int ok = sys_truncate("/" FILE_NAME, size) == 0;
    ui_printf("  truncate(/%s, %s)   ", FILE_NAME, label);
    ui_color(ok ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", ok ? "accepted!" : "refused (EFBIG)");
    ui_reset();
    return ok;
}

static int launch(void) {
    if (!vfs_finddir(fs_root, FILE_NAME) && !ramdisk_create(fs_root, FILE_NAME, FS_FILE)) {
        ui_printf("  could not create /%s\n", FILE_NAME);
        return 0;
    }
    int bad = try_size(0x7FFFFFFF, "2 GiB");
    bad += try_size(RAMDISK_MAX_FILE_SIZE + 1, "64 KiB + 1");
    ui_printf("  truncate(/%s, 100)        %s\n", FILE_NAME,
              sys_truncate("/" FILE_NAME, 100) == 0 ? "fine (a normal size)" : "refused?");
    return bad;
}

static int cleanup(void) {
    return vfs_finddir(fs_root, FILE_NAME) && ramdisk_delete(fs_root, FILE_NAME) == 0;
}

static const attack_t attack = {
    "huge_file",
    "truncate a file to 2 GiB: one allocation of the whole kernel heap",
    INV_RESOURCE, SEV_HIGH,
    simulate, launch, cleanup,
    "size_cap"
};

void atk_huge_file_register(void) { registry_add_attack(&attack); }
