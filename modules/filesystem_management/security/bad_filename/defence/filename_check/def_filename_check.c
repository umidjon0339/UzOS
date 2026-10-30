/*
 * def_filename_check.c - DEFENCE filename_check: only sane names.
 *
 * Stops:  bad_filename (security/attack/bad_filename)
 * Called: vfs_lookup / vfs_lookup_parent (core/vfs) for every component,
 *         ramdisk_create / ramdisk_move (core/ramdisk) for new names.
 * Rules:  component - 1 to FS_NAME_MAX characters (fits the name buffer)
 *         new name  - also: not "." or "..", no '/', only visible
 *                     characters (no spaces or control codes)
 * Why:    a 200-character component copied into a 128-byte buffer
 *         overflows it. A file called ".." or "a/b" can never be reached
 *         by a path, and one with spaces cannot be typed in the shell.
 */

#include "modules/filesystem_management/security/bad_filename/defence/filename_check/def_filename_check.h"  // this file
#include "modules/security_management/core/registry/registry.h"                                             // defence_t, registry_add_defence
#include "modules/security_management/core/audit/audit.h"                                                   // audit_log
#include "modules/filesystem_management/core/vfs/vfs.h"                                                     // FS_NAME_MAX
#include "lib/string/string.h"                                                                              // strcmp, memset

// The rules, as pure functions so the selftest can try them.
static int length_ok(int len) {
    return len >= 1 && len <= FS_NAME_MAX;
}

static int name_ok(const char *name) {
    int len = 0;
    while (name[len] && len <= FS_NAME_MAX)
        len++;
    if (!length_ok(len) || name[len])
        return 0;                               // empty or too long
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return 0;                               // reserved by paths
    for (int i = 0; i < len; i++)
        if (name[i] == '/' || name[i] <= ' ' || name[i] == 0x7F)
            return 0;                           // separator, space, control
    return 1;
}

int def_component_ok(int len) {
    if (length_ok(len))
        return 1;
    audit_log("filename_check", "path component longer than a name can be");
    return 0;
}

int def_new_name_ok(const char *name) {
    if (name && name_ok(name))
        return 1;
    audit_log("filename_check", "bad file name refused");
    return 0;
}

static int selftest(void) {
    char longest[FS_NAME_MAX + 1], too_long[FS_NAME_MAX + 2];
    memset(longest, 'x', FS_NAME_MAX);
    longest[FS_NAME_MAX] = '\0';
    memset(too_long, 'x', FS_NAME_MAX + 1);
    too_long[FS_NAME_MAX + 1] = '\0';

    if (!name_ok("notes.txt")) return 0;
    if (!name_ok(longest)) return 0;
    if (name_ok(too_long)) return 0;
    if (name_ok("")) return 0;
    if (name_ok(".") || name_ok("..")) return 0;
    if (name_ok("a/b")) return 0;
    if (name_ok("my file")) return 0;
    if (name_ok("bell\a")) return 0;
    if (!length_ok(FS_NAME_MAX) || length_ok(FS_NAME_MAX + 1)) return 0;
    return 1;
}

static const defence_t defence = {
    "filename_check",
    "bound every path component; refuse ., .., '/', spaces in new names",
    "bad_filename",
    selftest
};

void def_filename_check_register(void) {
    registry_add_defence(&defence);
}
