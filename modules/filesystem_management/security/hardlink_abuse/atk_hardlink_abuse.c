/*
 * atk_hardlink_abuse.c - ATTACK hardlink_abuse: keep a name on root's file.
 *
 * Breaks:     privilege - a user decides nothing about root's files
 * Stopped by: protected_hardlinks (defence/protected_hardlinks)
 *
 * A hard link is a second name for the SAME file. User 1 links root's
 * /seclab_secret into its home: when root deletes /seclab_secret, the
 * file lives on as user 1's name; and when root later runs "chown -R
 * guest /home/guest", it gives root's file away.
 *
 * simulate(): explain.
 * launch():   a user 1 process calls the real link("/seclab_secret",
 *             "/home/guest/mine"). findings = 1 if it got the name.
 * cleanup():  delete the link and /seclab_secret.
 *
 * Uses: sys_link / sys_unlink (syscalls/), ramdisk_* (core/ramdisk),
 *       process_create, sys_sleep, ui.
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

#define SECRET   "seclab_secret"
#define ATTACKER 1

static volatile int result, done;

static void linker(void) {
    result = sys_link("/" SECRET, "/home/guest/mine");
    done = 1;
}

static int simulate(void) {
    ui_printf("  user %d: ln /%s ~/mine   (a second name for root's file)\n", ATTACKER, SECRET);
    ui_printf("  root deletes /%s: it lives on; root chowns ~guest: it is given away\n", SECRET);
    return 1;
}

static int launch(void) {
    if (!vfs_finddir(fs_root, SECRET) && !ramdisk_create(fs_root, SECRET, FS_FILE))
        return 0;
    done = 0;
    if (process_create("linker", linker, ATTACKER) < 0)
        return 0;
    for (int i = 0; i < 50 && !done; i++)
        sys_sleep(10);
    int got = done && result == 0;
    ui_printf("  user %d: link(/%s, ~/mine)   ", ATTACKER, SECRET);
    ui_color(got ? UI_BAD : UI_GOOD);
    ui_printf("%s\n", got ? "a name on root's file!" : "refused (not its file)");
    ui_reset();
    return got;
}

static int cleanup(void) {
    int n = sys_unlink("/home/guest/mine") == 0;
    if (vfs_finddir(fs_root, SECRET))
        n += ramdisk_delete(fs_root, SECRET) == 0;
    return n;
}

static const attack_t attack = {
    "hardlink_abuse",
    "a user hard-links root's file into its own home",
    INV_PRIVILEGE, SEV_MEDIUM,
    simulate, launch, cleanup,
    "protected_hardlinks"
};

void atk_hardlink_abuse_register(void) { registry_add_attack(&attack); }
