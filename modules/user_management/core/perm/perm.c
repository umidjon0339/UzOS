/*
 * perm.c - Permission checks, and turning modes into text and back.
 *
 * Uses: process_caller_uid (process_management) to know who asks, and
 *       the defence file_access for the rule itself.
 */

#include "perm.h"
#include "process.h"
#include "def_file_access.h"

int perm_check(const vfs_node_t *node, int want) {
    if (!node)
        return 0;
    // DEFENCE file_access: owner digit for the owner, the other digit for
    // everybody else; root passes.
    return def_file_access_ok(process_caller_uid(), node->owner, node->mode, want);
}

int perm_may_remove(const vfs_node_t *dir, const vfs_node_t *node) {
    if (!perm_check(dir, PERM_WRITE))
        return 0;
    uint32_t me = process_caller_uid();
    // DEFENCE file_access: only the owner (or root) removes a file.
    return def_file_access_owner_ok(me, node->owner);
}

int perm_may_chmod(const vfs_node_t *node) {
    return node && def_file_access_owner_ok(process_caller_uid(), node->owner);
}

static void digit_text(uint32_t digit, char *out) {
    out[0] = digit & PERM_READ  ? 'r' : '-';
    out[1] = digit & PERM_WRITE ? 'w' : '-';
}

void perm_format(uint32_t mode, char out[6]) {
    digit_text(mode >> 3, out);         // owner: the first octal digit
    out[2] = '/';
    digit_text(mode & 7, out + 3);      // others: the second
    out[5] = '\0';
}

int perm_parse(const char *text, uint32_t *mode) {
    // Exactly two digits, each made of 4 and 2 only: 0, 2, 4, 6.
    for (int i = 0; i < 2; i++)
        if (text[i] < '0' || text[i] > '6' || (text[i] - '0') % 2)
            return 0;
    if (text[2] != '\0')
        return 0;
    *mode = (uint32_t)(text[0] - '0') << 3 | (uint32_t)(text[1] - '0');
    return 1;
}
