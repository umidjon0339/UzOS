/*
 * cmd_stat.c - `stat <path>...`: show a file's inode.
 *
 *     File: /docs/a.txt
 *     Size: 11           Blocks: 1     IO Block: 4096   regular file
 *   Device: 1  Inode: 7  Links: 1
 *   Access: (0644/-rw-r--r--)  Uid: (0/root)  Gid: (0/root)
 *   Access: 2026-10-30 12:00:01
 *   Modify: 2026-10-30 12:00:01
 *   Change: 2026-10-30 12:00:01
 *
 * It asks the stat() syscall, the same answer a program gets. UzOS's two
 * mode digits show as Linux's three (group = others; directories you
 * may read show x: you may enter them).
 *
 * Uses: sys_stat (syscalls/stat), vfs_lookup / vfs_path_of / vfs_cwd
 *       (core/vfs), users_name (user_management), arg_next, format, ui.
 */

#include "modules/filesystem_management/commands/stat/cmd_stat.h"   // this file
#include "modules/filesystem_management/syscalls/stat/sys_stat.h"   // sys_stat, vfs_stat_t
#include "modules/user_management/core/users/users.h"               // users_name
#include "lib/args/args.h"                                          // arg_next
#include "lib/format/format.h"                                      // format
#include "modules/ui/core/style/ui.h"                               // ui_printf, ui_error

// Seconds since 1970 -> "2026-10-30 12:00:01" (the civil calendar).
static void format_time(uint32_t t, char *out) {
    uint32_t days = t / 86400, secs = t % 86400;
    // Howard Hinnant's days -> date, for dates after 1970.
    uint32_t z = days + 719468, era = z / 146097, doe = z - era * 146097;
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    uint32_t mp = (5 * doy + 2) / 153;
    uint32_t day = doy - (153 * mp + 2) / 5 + 1;
    uint32_t month = mp < 10 ? mp + 3 : mp - 9;
    uint32_t year = yoe + era * 400 + (month <= 2);
    format(out, 24, "%04u-%02u-%02u %02u:%02u:%02u", year, month, day,
           secs / 3600, secs / 60 % 60, secs % 60);
}

static void rwx(uint32_t mode, char *out) {
    const char *t = (mode & S_IFMT) == S_IFDIR ? "d" : (mode & S_IFMT) == S_IFLNK ? "l" : "-";
    out[0] = t[0];
    for (int i = 0; i < 9; i++)
        out[1 + i] = (mode >> (8 - i)) & 1 ? "rwxrwxrwx"[i] : '-';
    out[10] = '\0';
}

static void show(const char *path) {
    vfs_stat_t st;
    if (sys_stat(path, &st) != 0) {
        ui_printf("stat: cannot stat '%s': no such file, or no permission\n", path);
        return;
    }
    char bits[11], owner[USER_NAME_MAX + 1], when[24];
    const char *kind = (st.mode & S_IFMT) == S_IFDIR ? "directory" :
                       (st.mode & S_IFMT) == S_IFLNK ? "symbolic link" : "regular file";
    rwx(st.mode, bits);
    users_name(st.uid, owner, sizeof(owner));

    ui_printf("  File: %s\n", path);
    ui_printf("  Size: %-10u Blocks: %-5u IO Block: %u   %s\n", st.size, st.blocks, st.blksize, kind);
    ui_printf("Device: %u  Inode: %u  Links: %u\n", st.dev, st.ino, st.nlink);
    ui_printf("Access: (%04o/%s)  Uid: (%u/%s)  Gid: (%u/%s)\n",
              st.mode & 07777, bits, st.uid, owner, st.gid, owner);
    format_time(st.atime, when);  ui_printf("Access: %s\n", when);
    format_time(st.mtime, when);  ui_printf("Modify: %s\n", when);
    format_time(st.ctime, when);  ui_printf("Change: %s\n", when);
}

void cmd_stat(const char *args) {
    char path[FS_PATH_MAX];
    const char *rest = arg_next(args, path, sizeof(path));
    if (!path[0]) {
        ui_usage("stat <path>...");
        return;
    }
    for (; path[0]; rest = arg_next(rest, path, sizeof(path)))
        show(path);
}
