#include "fs_security.h"
#include "proc_security.h"

int fs_security_check_path(const char *path) {
    if (!path) return 0;
    if (*path == '/') path++;

    if (!*path) return 0;

    int length = 0;
    while (path[length]) {
        if (path[length] == '/' || length >= FS_NAME_MAX) return 0;
        length++;
    }

    if ((length == 1 && path[0] == '.') ||
        (length == 2 && path[0] == '.' && path[1] == '.')) return 0;

    return 1;
}

static int def_path_component_selftest(void) {
    char max_name[FS_NAME_MAX + 1];
    char long_name[FS_NAME_MAX + 2];

    for (int i = 0; i < FS_NAME_MAX; i++) max_name[i] = 'x';
    max_name[FS_NAME_MAX] = '\0';
    for (int i = 0; i < FS_NAME_MAX + 1; i++) long_name[i] = 'x';
    long_name[FS_NAME_MAX + 1] = '\0';

    if (!fs_security_check_path("report")) return 0;
    if (!fs_security_check_path(max_name)) return 0;
    if (fs_security_check_path(long_name)) return 0;
    if (!fs_security_check_path("/report")) return 0;
    if (fs_security_check_path(0)) return 0;
    if (fs_security_check_path("")) return 0;
    if (fs_security_check_path("/")) return 0;
    if (fs_security_check_path("..")) return 0;
    if (fs_security_check_path("../secret")) return 0;
    if (fs_security_check_path("dir/secret")) return 0;
    if (fs_security_check_path(".")) return 0;
    return 1;
}

static const defence_t def = {
    "path_component",
    "allow only bounded single-name paths, not traversal or nested paths",
    "path_traversal",
    def_path_component_selftest
};

void def_path_component_register(void) {
    proc_security_register_defence(&def);
}
