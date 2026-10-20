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

#include "fs_commands.h"
#include "vfs.h"
#include "ui.h"
#include "box.h"
#include "perm.h"
#include "users.h"

#define NAME_WIDTH 24

static int is_program(vfs_node_t *n) {
    return n->size >= 4 && n->data && n->data[0] == 0x7F &&
           n->data[1] == 'E' && n->data[2] == 'L' && n->data[3] == 'F';
}

static void print_entry(vfs_node_t *n) {
    int dir = n->flags & FS_DIRECTORY;
    int program = !dir && is_program(n);

    vga_print_str("  ");
    ui_color(dir ? UI_LINK : program ? UI_GOOD : ui_text_fg());
    vga_print_str(dir ? GLYPH_FOLDER " " : program ? GLYPH_GEAR " " : GLYPH_FILE " ");

    char name[FS_NAME_MAX + 2];
    int len = 0;
    while (n->name[len] && len < FS_NAME_MAX) {
        name[len] = n->name[len];
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
    if (n->open_count)
        vga_print_str(" (open)");
    ui_end();
}

void cmd_ls(const char *args) {
    vfs_node_t *node = vfs_lookup(vfs_cwd(), args[0] ? args : ".");
    if (!node) {
        ui_error("ls", "no such file or directory");
        return;
    }

    if (node->flags & FS_FILE) {
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
