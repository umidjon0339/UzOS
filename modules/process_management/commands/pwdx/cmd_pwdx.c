/*
 * cmd_pwdx.c - `pwdx pid...`: where each process is working.
 *
 *   pwdx 2 5      2: /home/root
 *                 5: /tmp
 *
 * A process's directory says what its user is doing, so it is shown
 * only to its owner and root (hidepid).
 *
 * Uses: process_get_by_pid / process_caller_uid (core/process),
 *       vfs_path_of (filesystem_management), def_hidepid_visible
 *       (security/proc_info_leak/defence/hidepid), arg_next, atoi, ui.
 */

#include "modules/process_management/commands/pwdx/cmd_pwdx.h"       // this file
#include "modules/process_management/core/process/process.h"          // process_get_by_pid, process_caller_uid
#include "modules/process_management/security/proc_info_leak/defence/hidepid/def_hidepid.h"  // def_hidepid_visible
#include "modules/filesystem_management/core/vfs/vfs.h"               // vfs_path_of, FS_PATH_MAX
#include "lib/args/args.h"                                            // arg_next
#include "lib/stdlib/stdlib.h"                                        // atoi
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_usage

void cmd_pwdx(const char *args) {
    char word[12], path[FS_PATH_MAX];
    const char *rest = arg_next(args, word, sizeof(word));
    if (!word[0]) {
        ui_usage("pwdx pid...");
        return;
    }
    for (; word[0]; rest = arg_next(rest, word, sizeof(word))) {
        uint32_t pid = (uint32_t)atoi(word);
        process_t *p = process_get_by_pid(pid);
        if (!p) {
            ui_printf("%s: no such process\n", word);
        } else if (!def_hidepid_visible(process_caller_uid(), p)) {   // DEFENCE hidepid
            ui_printf("%u: permission denied\n", pid);
        } else {
            if (!p->cwd || vfs_path_of(p->cwd, path, sizeof(path)) != 0)
                path[0] = '/', path[1] = '\0';
            ui_printf("%u: %s\n", pid, path);
        }
    }
}
