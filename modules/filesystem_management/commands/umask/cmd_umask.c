/*
 * cmd_umask.c - `umask [mask]`: the bits a new file does NOT get.
 *
 *   umask          0022      group and others lose write (the default)
 *   umask 077                from now on: new files only for me
 *
 * It changes the shell's own umask (the umask() syscall), so every file
 * the following commands create, and every program the shell starts,
 * follows it. Whatever it says, a new file is never writable by others
 * (secure_umask): `umask 0` cannot hand out write rights.
 *
 * Uses: sys_umask (syscalls/umask), ui.
 */

#include "modules/filesystem_management/commands/umask/cmd_umask.h"   // this file
#include "modules/filesystem_management/syscalls/umask/sys_umask.h"   // sys_umask
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_usage

void cmd_umask(const char *args) {
    if (!args[0]) {
        uint32_t mask = sys_umask(0);           // read it by setting ...
        sys_umask(mask);                        // ... and putting it back
        ui_printf("%04o\n", mask);
        return;
    }
    uint32_t mask = 0;
    for (const char *c = args; *c; c++) {
        if (*c < '0' || *c > '7' || mask > 0777) {
            ui_usage("umask [octal mask, e.g. 022 or 077]");
            return;
        }
        mask = mask * 8 + (uint32_t)(*c - '0');
    }
    sys_umask(mask);
}
