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

#include "fs_commands.h"
#include "sys_open.h"
#include "sys_read.h"
#include "sys_write.h"
#include "sys_close.h"
#include "args.h"
#include "string.h"
#include "vfs.h"
#include "ui.h"

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
        ui_error("cp", "cannot open the source");
        return;
    }
    int out = sys_open(to, O_WRITE | O_CREATE | O_TRUNC);
    if (out < 0) {
        sys_close(in);
        ui_error("cp", "cannot create the destination");
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
