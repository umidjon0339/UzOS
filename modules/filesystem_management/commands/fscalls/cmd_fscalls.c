/*
 * cmd_fscalls.c - `fscalls [group]`: call the filesystem syscalls from
 * ring 3, check each answer.
 *
 * The same idea as `cpucalls` and `proccalls`: each GROUP is one ring 3
 * program (a .s file in this folder), run as user 1 (guest) through
 * usertask. It works in /home/guest, calls its syscalls with int 0x80,
 * and leaves every answer in the usertask mailbox; this command prints
 * them next to what a correct kernel answers. Some answers are refusals
 * on purpose (-1): another user's file, a size past size_cap, a time on
 * a file guest may not write.
 *
 *   fscalls            every group
 *   fscalls meta       one group: meta, dirs, links
 *
 * Uses: usertask_start / usertask_wait / usertask_mailbox
 *       (process_management/core/usertask), strcmp (lib/string), ui_* (ui).
 */

#include "modules/filesystem_management/commands/fscalls/cmd_fscalls.h"   // this file
#include "modules/process_management/core/usertask/usertask.h"           // usertask_start, usertask_wait, usertask_mailbox
#include "lib/string/string.h"                                           // strcmp
#include "modules/ui/core/style/ui.h"                                    // ui_printf, ui_color, ui_error

#define GUEST_UID 1
#define WAIT_MS   4000
#define DONE      0x516         // what each program writes to mailbox[0] at the end

typedef enum {
    EQ,         // exactly 'value'
    FD,         // a file descriptor (>= 3)
    AT_LEAST,   // 'value' or more
} expect_t;

typedef struct {
    const char *call;
    expect_t    expect;
    int32_t     value;
} check_t;

// From the .s programs in this folder, one per group
extern uint8_t meta_program[], meta_program_end[];
extern uint8_t dirs_program[], dirs_program_end[];
extern uint8_t links_program[], links_program_end[];

static const check_t meta_checks[] = {
    [1]  = { "open(meta1, create)",    FD,       0 },
    [2]  = { "write(\"hello\")",         EQ,       5 },
    [3]  = { "stat(meta1)",            EQ,       0 },
    [4]  = { "  st_size",              EQ,       5 },
    [5]  = { "  st_mode 0100644",      EQ,       0100644 },
    [6]  = { "  st_uid (guest)",       EQ,       1 },
    [7]  = { "  st_nlink",             EQ,       1 },
    [8]  = { "  st_ino",               AT_LEAST, 1 },
    [9]  = { "fstat(fd)",              EQ,       0 },
    [10] = { "  st_size",              EQ,       5 },
    [11] = { "fstatat(AT_FDCWD)",      EQ,       0 },
    [12] = { "statx(meta1)",           EQ,       0 },
    [13] = { "  stx_mask",             EQ,       0x7FF },
    [14] = { "lstat(meta1)",           EQ,       0 },
    [15] = { "access(meta1, R|W)",     EQ,       0 },
    [16] = { "access(/hello, W)",      EQ,       -1 },      // root's
    [17] = { "access(/hello, R)",      EQ,       0 },
    [18] = { "faccessat(meta1, R|W)",  EQ,       0 },
    [19] = { "chmod(meta1, 0600)",     EQ,       0 },
    [20] = { "  mode bits",            EQ,       0600 },
    [21] = { "chmod(/hello, 0666)",    EQ,       -1 },      // not the owner
    [22] = { "chown(meta1, root)",     EQ,       -1 },      // no CAP_CHOWN
    [23] = { "fchmod(fd, 0640)",       EQ,       0 },
    [24] = { "  mode bits (no group)", EQ,       0600 },
    [25] = { "umask(077): old",        EQ,       022 },
    [26] = { "open(meta2)",            FD,       0 },
    [27] = { "  meta2 mode bits",      EQ,       0600 },
    [28] = { "umask(0): old",          EQ,       066 },     // UzOS keeps r/w only
    [29] = { "  meta3: no others-w",   EQ,       0644 },    // secure_umask
    [30] = { "ftruncate(fd, 2)",       EQ,       0 },
    [31] = { "  st_size",              EQ,       2 },
    [32] = { "truncate(meta1, 10)",    EQ,       0 },
    [33] = { "  st_size",              EQ,       10 },
    [34] = { "truncate(meta1, 2 GiB)", EQ,       -1 },      // size_cap
    [35] = { "fallocate(fd, 0, 100)",  EQ,       0 },
    [36] = { "  st_size",              EQ,       100 },
    [37] = { "utimensat(1000, 2000)",  EQ,       0 },
    [38] = { "  st_atime",             EQ,       1000 },
    [39] = { "  st_mtime",             EQ,       2000 },
    [40] = { "utimensat(/hello, now)", EQ,       -1 },      // time_owner
    [41] = { "utime(meta1, now)",      EQ,       0 },
    [42] = { "utimes(meta1, 5, 6)",    EQ,       0 },
    [43] = { "  st_mtime",             EQ,       6 },
};

static const check_t dirs_checks[] = {
    [1]  = { "mkdirat(d, 0700)",       EQ,       0 },
    [2]  = { "  mode bits",            EQ,       0700 },
    [3]  = { "open(d, O_DIRECTORY)",   FD,       0 },
    [4]  = { "open(d/f1, create)",     FD,       0 },
    [5]  = { "mkdirat(dirfd, sub)",    EQ,       0 },
    [6]  = { "getdents64(dirfd)",      AT_LEAST, 1 },
    [7]  = { "  entries . .. f1 sub",  EQ,       4 },
    [8]  = { "getdents64: the end",    EQ,       0 },
    [9]  = { "fstatat(dirfd, f1)",     EQ,       0 },
    [10] = { "renameat(f1 -> f2)",     EQ,       0 },
    [11] = { "renameat2(NOREPLACE)",   EQ,       -1 },      // sub exists
    [12] = { "renameat2(EXCHANGE)",    EQ,       0 },
    [13] = { "  sub is a file now",    EQ,       8 },       // S_IFREG >> 12
    [14] = { "unlinkat(f2) a dir",     EQ,       -1 },      // EISDIR
    [15] = { "unlinkat(f2, REMOVEDIR)", EQ,      0 },
    [16] = { "unlink(d) a dir",        EQ,       -1 },
    [17] = { "rmdir(d) not empty",     EQ,       -1 },      // busy_check
    [18] = { "fchdir(dirfd)",          EQ,       0 },
    [19] = { "getcwd()",               EQ,       0 },
    [20] = { "  starts \"/hom\"",      EQ,       0x6D6F682F },
    [21] = { "unlinkat(cwd, sub)",     EQ,       0 },
    [22] = { "chdir(/home/guest)",     EQ,       0 },
    [23] = { "close(dirfd)",           EQ,       0 },
    [24] = { "rmdir(d)",               EQ,       0 },
    [25] = { "open(/hello, O_DIR)",    EQ,       -1 },      // not a directory
    [26] = { "open(dir, O_WRITE)",     EQ,       -1 },      // EISDIR
    [27] = { "open(/home/root, DIR)",  EQ,       -1 },      // private
    [28] = { "getdents64(a file)",     EQ,       -1 },
};

static const check_t links_checks[] = {
    [1]  = { "open(l1, create)",       FD,       0 },
    [2]  = { "write(\"abc\")",         EQ,       3 },
    [3]  = { "link(l1, l2)",           EQ,       0 },
    [4]  = { "  l2 nlink",             EQ,       2 },
    [5]  = { "  l1, l2: same inode",   EQ,       1 },
    [6]  = { "symlink(l1, s1)",        EQ,       0 },
    [7]  = { "readlink(s1)",           EQ,       2 },
    [8]  = { "  text \"l1\"",          EQ,       0x316C },
    [9]  = { "lstat(s1): a link",      EQ,       10 },      // S_IFLNK >> 12
    [10] = { "stat(s1): a file",       EQ,       8 },       // followed
    [11] = { "  its size",             EQ,       3 },
    [12] = { "readlink(l1) a file",    EQ,       -1 },
    [13] = { "link(/hello) root's",    EQ,       -1 },      // protected_hardlinks
    [14] = { "link(a directory)",      EQ,       -1 },
    [15] = { "symlink(loopa->loopb)",  EQ,       0 },
    [16] = { "symlink(loopb->loopa)",  EQ,       0 },
    [17] = { "stat(loopa) ELOOP",      EQ,       -1 },      // link_depth
    [18] = { "unlink(l1)",             EQ,       0 },
    [19] = { "  l2 nlink",             EQ,       1 },
    [20] = { "open(l2)",               FD,       0 },
    [21] = { "  read: still 3 bytes",  EQ,       3 },       // inode_refcount
    [22] = { "stat(s1) dangling",      EQ,       -1 },
    [23] = { "linkat(l2, l3)",         EQ,       0 },
    [24] = { "symlinkat(l2, s2)",      EQ,       0 },
    [25] = { "readlinkat(s2)",         EQ,       2 },
    [26] = { "linkat(s2, l4, FOLLOW)", EQ,       0 },
    [27] = { "  names of the file",    EQ,       3 },
};

static const struct {
    const char    *name;
    const uint8_t *start, *end;
    const check_t *checks;
    int            count;           // slots 1..count
} groups[] = {
    { "meta", meta_program, meta_program_end, meta_checks,
      sizeof(meta_checks) / sizeof(meta_checks[0]) - 1 },
    { "dirs", dirs_program, dirs_program_end, dirs_checks,
      sizeof(dirs_checks) / sizeof(dirs_checks[0]) - 1 },
    { "links", links_program, links_program_end, links_checks,
      sizeof(links_checks) / sizeof(links_checks[0]) - 1 },
};
#define GROUP_COUNT (int)(sizeof(groups) / sizeof(groups[0]))

static int total_good, total_count;    // over every group run by one command

static int as_expected(const check_t *c, int32_t got) {
    switch (c->expect) {
        case FD:       return got >= 3;
        case AT_LEAST: return got >= c->value;
        default:       return got == c->value;
    }
}

// Run one group's program and print its answers.
static void run_group(int g) {
    volatile uint32_t *box = usertask_mailbox();
    int pid = usertask_start(groups[g].name, groups[g].start, groups[g].end, GUEST_UID);
    if (pid < 0) {
        ui_error("fscalls", "could not start the ring 3 program");
        return;
    }
    usertask_wait(pid, WAIT_MS);
    if (box[0] != DONE) {
        ui_error("fscalls", "the program did not finish");
        return;
    }

    int good = 0, count = groups[g].count;
    ui_printf("\n%s: ring 3 program PID %d, user guest\n", groups[g].name, pid);
    for (int slot = 1; slot <= count; slot++) {
        const check_t *c = &groups[g].checks[slot];
        int32_t got = (int32_t)box[slot];
        int ok = as_expected(c, got);
        good += ok;
        ui_printf("%-23s%6d ", c->call, got);
        ui_color(ok ? UI_GOOD : UI_BAD);
        vga_print_str(ok ? "ok" : "NO");
        ui_reset();
        vga_print_str(slot % 2 ? "    " : "\n");      // two columns
    }
    vga_print_str(count % 2 ? "\n" : "");
    ui_start(good == count ? UI_OK : UI_ERROR, "fscalls");
    ui_printf("%s: %d of %d answers as expected", groups[g].name, good, count);
    ui_end();
    total_good += good;
    total_count += count;
}

void cmd_fscalls(const char *args) {
    int ran = 0;
    total_good = total_count = 0;
    for (int g = 0; g < GROUP_COUNT; g++) {
        if (args[0] && strcmp(args, groups[g].name) != 0)
            continue;
        run_group(g);
        ran++;
    }
    if (!ran) {
        ui_usage("fscalls [meta|dirs|links]");
    } else if (ran > 1) {
        ui_start(total_good == total_count ? UI_OK : UI_ERROR, "fscalls");
        ui_printf("all %d groups: %d of %d answers as expected", ran, total_good, total_count);
        ui_end();
    }
}
