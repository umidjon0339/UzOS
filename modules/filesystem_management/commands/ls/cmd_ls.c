/*
 * cmd_ls.c - `ls [path]`: list a directory (or show one file).
 *
 *     ■ docs/                  -  rw/r-  guest
 *     ▪ hello               1234 B  rw/r-  root    program
 *     ▪ notes.txt             42 B  rw/--  guest   (open)
 *
 * Each entry gets an icon and colour: directories blue, programs (files
 * that start with the ELF signature 7F 'E' 'L' 'F') green. Then its
 * mode (owner/others, see `chmod`) and its owner. Listing a directory
 * needs read permission on it.
 *
 * Uses: vfs_lookup / vfs_readdir (core/vfs), perm_check / perm_format and
 *       users_name (user_management), ui_* / ui_cell (ui module).
 */

#include "modules/filesystem_management/commands/ls/cmd_ls.h"  // this file
#include "modules/filesystem_management/core/vfs/vfs.h"        // vfs_node_t, FS_DIRECTORY, FS_NAME_MAX
#include "modules/ui/core/style/ui.h"                          // ui_color, UI_LINK, UI_GOOD
#include "modules/ui/core/box/box.h"                           // ui_cell
#include "modules/user_management/core/perm/perm.h"            // perm_format, perm_check, PERM_READ
#include "modules/user_management/core/users/users.h"          // USER_NAME_MAX, users_name

#define NAME_WIDTH 24

static int is_program(vfs_node_t *n) {
    return n->size >= 4 && n->data && n->data[0] == 0x7F &&
           n->data[1] == 'E' && n->data[2] == 'L' && n->data[3] == 'F';
}

// 'entry' is the name; its inode (a hard link's file) has the facts.
static void print_entry(vfs_node_t *entry) {
    vfs_node_t *n = vfs_inode(entry);
    int dir = n->flags & FS_DIRECTORY;
    int link = (n->flags & FS_LINK) != 0;
    int program = !dir && !link && is_program(n);

    vga_print_str("  ");
    ui_color(dir ? UI_LINK : program ? UI_GOOD : ui_text_fg());
    vga_print_str(dir ? GLYPH_FOLDER " " : program ? GLYPH_GEAR " " : GLYPH_FILE " ");

    char name[FS_NAME_MAX + 2];
    int len = 0;
    while (entry->name[len] && len < FS_NAME_MAX) {
        name[len] = entry->name[len];
        len++;
    }
    if (dir)
        name[len++] = '/';
    name[len] = '\0';
    ui_cell(name, NAME_WIDTH);

    ui_reset();
    if (dir)
        ui_printf("%8s  ", "-");
    else
        ui_printf("%6u B  ", n->size);

    char mode[6], owner[USER_NAME_MAX + 2];
    perm_format(n->mode, mode);
    users_name(n->owner, owner, sizeof(owner));
    ui_printf("%s  ", mode);
    ui_cell(owner, 7);
    ui_color(UI_DIM);
    if (program)
        vga_print_str(" program");
    if (link) {                                 // where a symbolic link points
        char target[FS_PATH_MAX];
        uint32_t t = n->size < FS_PATH_MAX - 1 ? n->size : FS_PATH_MAX - 1;
        for (uint32_t i = 0; i < t; i++)
            target[i] = (char)n->data[i];
        target[t] = '\0';
        ui_printf(" -> %s", target);
    }
    if (n->nlink > 1)
        ui_printf(" (%u names)", n->nlink);
    if (n->open_count)
        vga_print_str(" (open)");
    ui_end();
}

void cmd_ls(const char *args) {
    vfs_node_t *node = vfs_lookup(vfs_cwd(), args[0] ? args : ".");
    if (node && !(node->flags & FS_DIRECTORY))  // one file (or link): show the name given
        node = vfs_lookup_nofollow(vfs_cwd(), args);
    if (!node) {
        ui_error("ls", "no such file or directory");
        return;
    }

    if (!(node->flags & FS_DIRECTORY)) {
        print_entry(node);
        return;
    }
    if (!perm_check(node, PERM_READ)) {
        ui_error("ls", "permission denied");
        return;
    }

    uint32_t count = 0;
    for (vfs_node_t *child; (child = vfs_readdir(node, count)); count++)
        print_entry(child);
    if (count == 0)
        ui_hint("  (empty)");
}
