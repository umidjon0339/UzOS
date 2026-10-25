/*
 * atk_file_permission_bypass.c - ATTACK file_permission_bypass: use files that are not yours.
 *
 * Breaks:     isolation - a user's files are theirs alone
 * Stopped by: file_access (security/defence/file_access)
 *
 * Without permission checks, a guest program could read root's private
 * notes, overwrite root's files, delete them, or fill root's directories.
 *
 * simulate(): list the four tries and what each would do.
 * launch():   as root, make two files: /home/root/secret (mode 60,
 *             private) and /seclab_root_file (mode 64). Then run
 *             file_permission_bypass.s as guest in ring 3: it tries all
 *             four through the real syscalls.
 * cleanup():  remove both files (and /seclab_evil, if it was made).
 *
 * Uses: ramdisk_create / ramdisk_delete / vfs_* (filesystem_management),
 *       MODE_PRIVATE (core/perm), usertask_* (process_management).
 */

#include "registry.h"
#include "usertask.h"
#include "ramdisk.h"
#include "perm.h"
#include "users.h"
#include "string.h"
#include "ui.h"

#define WAIT_MS 300
#define DONE    0x600D          // what the program writes to mailbox[0]

// From file_permission_bypass.s
extern uint8_t perm_program[], perm_program_end[];

// In the order the program tries them; mailbox[i + 1] holds the answer.
static const char *const tries[] = {
    "read  /home/root/secret      (root only, mode 60)",
    "write /seclab_root_file      (root's, mode 64)",
    "rm    /seclab_root_file",
    "mkdir /seclab_evil           (inside root's /)",
};
#define TRY_COUNT (sizeof(tries) / sizeof(tries[0]))

static int simulate(void) {
    for (unsigned i = 0; i < TRY_COUNT; i++)
        ui_printf("  guest: %s\n", tries[i]);
    ui_printf("  without checks every one works: nothing would belong to anyone\n");
    return TRY_COUNT;
}

// A file owned by root (the caller: live attacks need root) with 'mode'.
static void make_root_file(vfs_node_t *dir, const char *name, uint32_t mode) {
    vfs_node_t *file = ramdisk_create(dir, name, FS_FILE);
    if (!file)
        return;
    const char *text = "root's text\n";
    vfs_write(file, 0, strlen(text), (uint8_t *)text);
    file->mode = mode;
}

static int cleanup(void) {
    vfs_node_t *home = vfs_lookup(fs_root, "/home/root");
    int removed = 0;
    removed += home && ramdisk_delete(home, "secret") == 0;
    removed += ramdisk_delete(fs_root, "seclab_root_file") == 0;
    removed += ramdisk_delete(fs_root, "seclab_evil") == 0;
    return removed;
}

static int launch(void) {
    volatile uint32_t *mailbox = usertask_mailbox();

    cleanup();                              // start from a clean state
    make_root_file(vfs_lookup(fs_root, "/home/root"), "secret", MODE_PRIVATE);
    make_root_file(fs_root, "seclab_root_file", MODE_DEFAULT);

    int pid = usertask_start("snoop", perm_program, perm_program_end, UID_GUEST);
    if (pid < 0) {
        ui_printf("  could not start the ring 3 program\n");
        return 0;
    }
    usertask_wait(pid, WAIT_MS);
    if (mailbox[0] != DONE) {
        ui_printf("  the program did not finish\n");
        return 0;
    }

    int worked = 0;
    for (unsigned i = 0; i < TRY_COUNT; i++) {
        int ok = (int)mailbox[i + 1] >= 0;
        worked += ok;
        ui_printf("  %s  ", tries[i]);
        ui_color(ok ? UI_BAD : UI_GOOD);
        ui_printf("%s\n", ok ? "ALLOWED" : "denied");
        ui_reset();
    }
    return worked;
}

static const attack_t attack = {
    "file_permission_bypass",
    "a guest program reads, overwrites and deletes root's files",
    INV_ISOLATION, SEV_HIGH,
    simulate, launch, cleanup,
    "file_access"
};

void atk_file_permission_bypass_register(void) {
    registry_add_attack(&attack);
}
