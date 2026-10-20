/*
 * atk_fd_leak.c - ATTACK fd_leak: end a process with files still open.
 *
 * Breaks:     resource - open counts must come back down
 * Stopped by: fd_cleanup (security/defence/fd_cleanup)
 *
 * A process opens a file and ends without close(). Its descriptor held
 * the file's open count up; if nobody closes it, the file can never be
 * deleted (busy_check refuses).
 *
 * simulate(): explain the leak.
 * launch():   create /seclab_leak, start a process that opens it and
 *             returns at once, wait for it to end and be reaped, then
 *             check the open count and try to delete the file.
 *
 * Uses: process_create (process_management), sys_open (syscalls/open),
 *       ramdisk_create / sys_unlink (filesystem_management), timer_wait.
 */

#include "security_lab.h"
#include "process.h"
#include "ramdisk.h"
#include "sys_open.h"
#include "sys_unlink.h"
#include "timer.h"
#include "vga.h"

#define TEMP       "/seclab_leak"
#define WAIT_TICKS 20               // 0.2 s: the process runs, ends, is reaped

static void leaky_process(void) {
    sys_open(TEMP, O_READ);         // ...and never close it
}

static int simulate(void) {
    vga_print_str("  a process opens a file, then ends without close()\n");
    vga_print_str("  its descriptor keeps the file's open count at 1 forever\n");
    vga_print_str("  -> rm of that file is refused for the rest of the uptime\n");
    return 1;
}

static int launch(void) {
    vfs_node_t *file = ramdisk_create(fs_root, TEMP + 1, FS_FILE);
    if (!file)
        return 0;
    uint32_t denied = sec_total_denied();

    process_create("leaky", leaky_process, 0);
    timer_wait(WAIT_TICKS);

    vga_print_str("  open count after the process ended: ");
    vga_print_int(file->open_count);
    vga_print_str(file->open_count ? "  (LEAKED)\n" : "  (closed for it)\n");

    int deleted = sys_unlink(TEMP) == 0;
    vga_print_str(deleted ? "  rm /seclab_leak -> deleted\n" : "  rm /seclab_leak -> REFUSED, stuck\n");
    return sec_total_denied() - denied;
}

static const attack_t attack = {
    "fd_leak",
    "a process ends without closing its files; they stay 'open' forever",
    INV_RESOURCE, SEV_MEDIUM,
    simulate, launch, 0,        // launch deletes the file when it can
    "fd_cleanup"
};

void atk_fd_leak_register(void) {
    proc_security_register_attack(&attack);
}
