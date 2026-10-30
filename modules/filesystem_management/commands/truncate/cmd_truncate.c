/*
 * cmd_truncate.c - `truncate -s SIZE <path>...`: set a file's length.
 *
 *   truncate -s 0 log         empty it
 *   truncate -s 4K blank      4096 zero bytes (K = 1024)
 *
 * A file that does not exist is created first (as Linux does). The
 * size goes through size_cap (64 KiB, RLIMIT_FSIZE) in the syscall.
 *
 * Uses: sys_truncate (syscalls/truncate), sys_open / sys_close
 *       (io_management) to create, arg_next, ui.
 */

#include "modules/filesystem_management/commands/truncate/cmd_truncate.h"   // this file
#include "modules/filesystem_management/syscalls/truncate/sys_truncate.h"   // sys_truncate
#include "modules/io_management/syscalls/open/sys_open.h"                   // sys_open, O_CREATE
#include "modules/io_management/syscalls/close/sys_close.h"                 // sys_close
#include "modules/filesystem_management/core/vfs/vfs.h"                     // FS_PATH_MAX
#include "lib/args/args.h"                                                  // arg_next
#include "lib/string/string.h"                                              // strcmp
#include "modules/ui/core/style/ui.h"                                       // ui_printf, ui_usage

// "4096", "4K", "1M" -> bytes. 0 = bad (a size of 0 is spelled "0").
static int parse_size(const char *text, uint32_t *out) {
    uint32_t n = 0;
    const char *c = text;
    for (; *c >= '0' && *c <= '9'; c++) {
        if (n > 0x0FFFFFFF)
            return 0;
        n = n * 10 + (uint32_t)(*c - '0');
    }
    if (c == text)
        return 0;
    if (*c == 'K' && n <= 0x3FFFFF)
        n *= 1024, c++;
    else if (*c == 'M' && n <= 0xFFF)
        n *= 1024 * 1024, c++;
    if (*c)
        return 0;
    *out = n;
    return 1;
}

void cmd_truncate(const char *args) {
    char word[FS_PATH_MAX];
    uint32_t size;
    const char *rest = arg_next(args, word, sizeof(word));
    if (strcmp(word, "-s") != 0) {
        ui_usage("truncate -s SIZE <path>...");
        return;
    }
    rest = arg_next(rest, word, sizeof(word));
    if (!parse_size(word, &size)) {
        ui_usage("truncate -s SIZE <path>...   (SIZE: 100, 4K, 1M)");
        return;
    }
    for (rest = arg_next(rest, word, sizeof(word)); word[0]; rest = arg_next(rest, word, sizeof(word))) {
        int fd = sys_open(word, O_WRITE | O_CREATE);    // exists afterwards
        if (fd >= 0)
            sys_close(fd);
        if (fd < 0 || sys_truncate(word, size) != 0)
            ui_printf("truncate: %s: refused (permission, or too big: see `audit`)\n", word);
    }
}
