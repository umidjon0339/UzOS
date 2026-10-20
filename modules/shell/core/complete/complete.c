/*
 * complete.c - Find what the word being typed could be, and finish it.
 *
 * Uses: the command registry (core/shell), vfs_lookup / vfs_readdir
 *       (filesystem_management), the line buffer (core/line), ui colours.
 */

#include "complete.h"
#include "shell.h"
#include "vfs.h"
#include "ui.h"
#include "string.h"

#define MAX_CANDIDATES 64

static const char *candidates[MAX_CANDIDATES];
static int is_dir[MAX_CANDIDATES];
static int found;

static int starts_with(const char *s, const char *prefix) {
    while (*prefix)
        if (*s++ != *prefix++)
            return 0;
    return 1;
}

static void add_candidate(const char *name, int dir) {
    if (found < MAX_CANDIDATES) {
        candidates[found] = name;
        is_dir[found] = dir;
        found++;
    }
}

// How many characters all candidates share.
static int common_length(void) {
    int len = strlen(candidates[0]);
    for (int i = 1; i < found; i++) {
        int j = 0;
        while (j < len && candidates[i][j] == candidates[0][j])
            j++;
        len = j;
    }
    return len;
}

static void list_candidates(void) {
    vga_print_str("\n");
    for (int i = 0; i < found; i++) {
        ui_color(is_dir[i] ? UI_LINK : UI_TITLE);   // directories like in ls
        vga_print_str(candidates[i]);
        vga_print_str(is_dir[i] ? "/  " : "  ");
    }
    ui_end();
}

complete_result_t complete_line(line_t *line) {
    // The word being typed starts after the last space.
    int word = line->length;
    while (word > 0 && line->text[word - 1] != ' ')
        word--;
    const char *typed = &line->text[word];
    int first_word = (word == 0);

    // For a path, only the part after the last '/' is completed.
    const char *prefix = typed;
    found = 0;

    if (first_word) {
        for (int i = 0; i < shell_command_count(); i++)
            if (starts_with(shell_command(i)->name, typed))
                add_candidate(shell_command(i)->name, 0);
    } else {
        const char *slash = strrchr(typed, '/');
        char dir_path[LINE_MAX];
        if (slash) {
            int dir_len = slash - typed + 1;            // keep the '/' ("/" is the root)
            memcpy(dir_path, typed, dir_len);
            dir_path[dir_len] = '\0';
            prefix = slash + 1;
        } else {
            strcpy(dir_path, ".");
        }
        vfs_node_t *dir = vfs_lookup(vfs_cwd(), dir_path);
        vfs_node_t *child;
        for (uint32_t i = 0; (child = vfs_readdir(dir, i)); i++)
            if (starts_with(child->name, prefix))
                add_candidate(child->name, child->flags & FS_DIRECTORY);
    }

    if (found == 0)
        return COMPLETE_NOTHING;

    // Add the characters every match agrees on.
    int typed_len = strlen(prefix);
    int shared = common_length();
    for (int i = typed_len; i < shared; i++)
        line_insert(line, candidates[0][i]);

    if (found == 1) {
        line_insert(line, is_dir[0] ? '/' : ' ');       // ready for the next part
        return COMPLETE_ADDED;
    }
    if (shared > typed_len)
        return COMPLETE_ADDED;

    list_candidates();                                  // nothing to add: show them
    return COMPLETE_LISTED;
}
