/*
 * cmd_cp.c - `cp <from> <to>`: copy a file.
 *
 * Read a chunk from one descriptor, write it to the other, repeat.
 * <to> may be a directory: the copy then keeps the original name.
 *
 * Uses: sys_open / sys_read / sys_write / sys_close (io_management),
 *       vfs_lookup / vfs_path_of (core/vfs), arg_next (lib/args),
 *       ui_* (ui module).
 */

#include "modules/filesystem_management/commands/cp/cmd_cp.h"  // this file
#include "modules/io_management/syscalls/open/sys_open.h"      // sys_open, O_READ, O_WRITE
#include "modules/io_management/syscalls/read/sys_read.h"      // sys_read
#include "modules/io_management/syscalls/write/sys_write.h"    // sys_write
#include "modules/io_management/syscalls/close/sys_close.h"    // sys_close
#include "lib/args/args.h"                                     // arg_next
#include "lib/string/string.h"                                 // strlen, strcpy
#include "modules/filesystem_management/core/vfs/vfs.h"        // vfs_node_t, vfs_path_of, FS_PATH_MAX
#include "modules/ui/core/style/ui.h"                          // ui_usage, ui_error

#define CHUNK 512

// "cp notes docs" means "cp notes docs/notes": write dir's path + "/" + name
// into 'to'. Returns 0 if it would not fit.
static int path_inside(char *to, int max, vfs_node_t *dir, const char *name) {
    if (vfs_path_of(dir, to, max) != 0)
        return 0;
    int len = strlen(to);
    if (len + 1 + (int)strlen(name) + 1 > max)
        return 0;
    if (len > 1)                    // "/" already ends in a slash
        to[len++] = '/';
    strcpy(to + len, name);
    return 1;
}

void cmd_cp(const char *args) {
    char from[FS_PATH_MAX], to[FS_PATH_MAX];
    arg_next(arg_next(args, from, sizeof(from)), to, sizeof(to));
    if (!from[0] || !to[0]) {
        ui_usage("cp <from> <to>");
        return;
    }

    vfs_node_t *src = vfs_lookup(vfs_cwd(), from);
    vfs_node_t *dst = vfs_lookup(vfs_cwd(), to);
    if (src && dst && (dst->flags & FS_DIRECTORY) &&
        !path_inside(to, sizeof(to), dst, src->name)) {
        ui_error("cp", "destination path too long");
        return;
    }

    int in = sys_open(from, O_READ);
    if (in < 0) {
        ui_error("cp", "cannot open the source (missing, or permission denied)");
        return;
    }
    int out = sys_open(to, O_WRITE | O_CREATE | O_TRUNC);
    if (out < 0) {
        sys_close(in);
        ui_error("cp", "cannot create the destination (permission denied?)");
        return;
    }

    char buf[CHUNK];
    int n;
    while ((n = sys_read(in, buf, CHUNK)) > 0)
        if (sys_write(out, buf, n) != n) {
            ui_error("cp", "write failed (file too big?)");
            break;
        }
    sys_close(in);
    sys_close(out);
}
