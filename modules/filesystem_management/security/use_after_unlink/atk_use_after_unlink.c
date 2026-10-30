/*
 * atk_use_after_unlink.c - ATTACK use_after_unlink: read a file the kernel
 * already freed.
 *
 * Breaks:     consistency - a name or a descriptor always leads to live memory
 * Stopped by: inode_refcount (defence/inode_refcount)
 *
 * With hard links a file has several names, and descriptors refer to the
 * file, not to a name. If removing ONE name freed the file, the other
 * names and every open descriptor would point at freed kernel memory: a
 * read shows whatever the heap holds next, a write corrupts it.
 *
 * simulate(): explain.
 * launch():   /seclab_keep holds "STILL HERE", /seclab_keep2 is a hard
 *             link. unlink(keep), read keep2; open keep2, unlink(keep2),
 *             read the descriptor; close it. findings = reads that did
 *             not give "STILL HERE".
 * cleanup():  delete what is left.
 *
 * Uses: sys_link / sys_unlink (syscalls/), sys_open / sys_read /
 *       sys_close (io_management), ramdisk_* (core/ramdisk), ui.
 */
#include "modules/security_management/core/registry/registry.h"   // attack_t, registry_add_attack
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"   // ramdisk_create, ramdisk_delete
#include "modules/filesystem_management/core/vfs/vfs.h"           // fs_root, vfs_finddir, vfs_write
#include "modules/process_management/core/process/process.h"      // process_create
#include "modules/process_management/syscalls/sleep/sys_sleep.h"  // sys_sleep
#include "lib/string/string.h"                                    // strcpy, strncmp
#include "modules/ui/core/style/ui.h"                             // ui_printf, ui_color
#include "modules/filesystem_management/syscalls/link/sys_link.h"       // sys_link
#include "modules/filesystem_management/syscalls/unlink/sys_unlink.h"   // sys_unlink
#include "modules/io_management/syscalls/open/sys_open.h"               // sys_open, O_READ
#include "modules/io_management/syscalls/read/sys_read.h"               // sys_read
#include "modules/io_management/syscalls/close/sys_close.h"             // sys_close

#define TEXT "STILL HERE"

static int check(const char *what, int fd_or_neg, int ok) {
    (void)fd_or_neg;
    ui_printf("  %-38s ", what);
    ui_color(ok ? UI_GOOD : UI_BAD);
    ui_printf("%s\n", ok ? "\"" TEXT "\"" : "freed memory!");
    ui_reset();
    return !ok;
}

static int read_text(int fd) {
    char buf[16] = { 0 };
    return fd >= 0 && sys_read(fd, buf, 10) == 10 && strncmp(buf, TEXT, 10) == 0;
}

static int simulate(void) {
    ui_printf("  ln keep keep2 ; rm keep ; cat keep2        (one name gone, the file not)\n");
    ui_printf("  exec 3<keep2 ; rm keep2 ; read fd 3        (no name, still open)\n");
    return 2;
}

static int launch(void) {
    vfs_node_t *f = vfs_finddir(fs_root, "seclab_keep");
    if (!f)
        f = ramdisk_create(fs_root, "seclab_keep", FS_FILE);
    if (!f || vfs_write(f, 0, 10, (uint8_t *)TEXT) != 10 || sys_link("/seclab_keep", "/seclab_keep2") != 0)
        return 0;

    int bad = 0;
    sys_unlink("/seclab_keep");
    int fd = sys_open("/seclab_keep2", O_READ);
    bad += check("unlink(keep), read keep2", fd, read_text(fd));
    if (fd >= 0)
        sys_close(fd);

    fd = sys_open("/seclab_keep2", O_READ);
    sys_unlink("/seclab_keep2");                    // no name left; still open
    bad += check("unlink(keep2) while open, read the fd", fd, read_text(fd));
    if (fd >= 0)
        sys_close(fd);                              // now it is freed
    ui_printf("  closed: no name, no fd -> freed now\n");
    return bad;
}

static int cleanup(void) {
    int n = 0;
    if (vfs_finddir(fs_root, "seclab_keep"))  n += ramdisk_delete(fs_root, "seclab_keep") == 0;
    if (vfs_finddir(fs_root, "seclab_keep2")) n += ramdisk_delete(fs_root, "seclab_keep2") == 0;
    return n;
}

static const attack_t attack = {
    "use_after_unlink",
    "remove one name of a file; the other names and fds point at freed memory",
    INV_CONSISTENCY, SEV_HIGH,
    simulate, launch, cleanup,
    "inode_refcount"
};

void atk_use_after_unlink_register(void) { registry_add_attack(&attack); }
