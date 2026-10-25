/*
 * def_busy_check.c - DEFENCE busy_check: never delete something in use.
 *
 * Stops:  delete_open_file (security/attack/delete_open_file)
 * Called: ramdisk_delete() (core/ramdisk), before anything is freed.
 * Rules:  a node may be deleted only if
 *           1. no descriptor has it open        (open_count == 0)
 *           2. it is not a directory with entries
 *           3. it is not the root
 *           4. no process has it as its current directory
 * Why:    deleting frees the node's memory. A descriptor or a cwd still
 *         pointing at it would then use freed memory ("use after free"):
 *         the next kmalloc() reuses that memory for something else, and
 *         reading the "file" reads, or writes, that something else.
 */

#include "def_busy_check.h"
#include "registry.h"
#include "audit.h"
#include "process.h"

static int is_some_cwd(vfs_node_t *node) {
    process_t *table = process_get_table();
    for (int i = 0; i < MAX_PROCESSES; i++)
        if (table[i].state != PROCESS_STATE_FREE && table[i].cwd == node)
            return 1;
    return 0;
}

// Why deleting would be wrong, or 0 if it is fine. A pure function.
static const char *refusal(uint32_t open_count, int is_dir, int children,
                           int is_root, int is_cwd) {
    if (is_root)        return "delete refused: the root directory";
    if (open_count)     return "delete refused: file is still open";
    if (is_dir && children) return "delete refused: directory is not empty";
    if (is_cwd)         return "delete refused: a process is inside this directory";
    return 0;
}

int def_delete_allowed(vfs_node_t *node) {
    const char *reason = refusal(node->open_count, node->flags & FS_DIRECTORY,
                                 node->num_children, node == fs_root, is_some_cwd(node));
    if (!reason)
        return 1;
    audit_log("busy_check", reason);
    return 0;
}

static int selftest(void) {
    if (refusal(0, 0, 0, 0, 0) != 0) return 0;      // closed file: ok
    if (refusal(0, 1, 0, 0, 0) != 0) return 0;      // empty directory: ok
    if (refusal(1, 0, 0, 0, 0) == 0) return 0;      // open file
    if (refusal(0, 1, 3, 0, 0) == 0) return 0;      // directory with entries
    if (refusal(0, 1, 0, 1, 0) == 0) return 0;      // the root
    if (refusal(0, 1, 0, 0, 1) == 0) return 0;      // someone's cwd
    return 1;
}

static const defence_t defence = {
    "busy_check",
    "never delete an open file, a non-empty directory, or a current directory",
    "delete_open_file",
    selftest
};

void def_busy_check_register(void) {
    registry_add_defence(&defence);
}
