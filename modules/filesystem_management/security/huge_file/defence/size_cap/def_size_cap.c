/*
 * def_size_cap.c - DEFENCE size_cap: a file cannot be made bigger than the
 * disk allows by setting its size.
 *
 * Stops:  huge_file (security/huge_file)
 * Called: sys_truncate, sys_ftruncate, sys_fallocate (syscalls/), before
 *         ramdisk_resize() allocates anything.
 * Rules:  1. a size above RAMDISK_MAX_FILE_SIZE (64 KiB) is refused
 *         2. a size above the caller's RLIMIT_FSIZE (`ulimit -f`) is refused
 * Why:    write() grows a file a few bytes at a time and file_range
 *         stops it at the end. truncate(file, 0x7FFFFFFF) asks for 2 GiB
 *         in ONE call: on a RAM disk that is a single kmalloc of the
 *         whole kernel heap - every other file, process stack and
 *         buffer then fails. (Linux: EFBIG, and RLIMIT_FSIZE sends
 *         SIGXFSZ.)
 *
 * Uses: RAMDISK_MAX_FILE_SIZE (core/ramdisk), rlimit_fsize_ok
 *       (cpu_management core/rlimit), scheduler_get_current_process,
 *       audit_log, registry.
 */

#include "modules/filesystem_management/security/huge_file/defence/size_cap/def_size_cap.h"  // this file
#include "modules/filesystem_management/core/ramdisk/ramdisk.h"       // RAMDISK_MAX_FILE_SIZE
#include "modules/cpu_management/core/rlimit/rlimit.h"                // rlimit_fsize_ok
#include "modules/process_management/core/scheduler/scheduler.h"      // scheduler_get_current_process
#include "modules/security_management/core/registry/registry.h"       // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"             // audit_log

static int within_disk(uint32_t size) {
    return size <= RAMDISK_MAX_FILE_SIZE;
}

int def_new_size_ok(uint32_t size) {
    if (!within_disk(size)) {
        audit_log("size_cap", "file size refused: larger than a file may be");
        return 0;
    }
    if (!rlimit_fsize_ok(scheduler_get_current_process(), size)) {
        audit_log("size_cap", "file size refused: over RLIMIT_FSIZE");
        return 0;
    }
    return 1;
}

static int selftest(void) {
    if (!within_disk(0)) return 0;
    if (!within_disk(RAMDISK_MAX_FILE_SIZE)) return 0;
    if (within_disk(RAMDISK_MAX_FILE_SIZE + 1)) return 0;
    if (within_disk(0x7FFFFFFF)) return 0;
    return 1;
}

static const defence_t defence = {
    "size_cap",
    "truncate / fallocate: never past 64 KiB or RLIMIT_FSIZE",
    "huge_file",
    selftest
};

void def_size_cap_register(void) { registry_add_defence(&defence); }
