/*
 * cmd_pgrep.c - `pgrep [-l] [-x] [-u user] pattern`: find processes by name.
 *
 *   pgrep work          PIDs of every process whose name CONTAINS "work"
 *   pgrep -x worker     ...whose name IS "worker"
 *   pgrep -l -u guest . guest's processes, with their names
 *
 * Linux's pgrep takes a regular expression; UzOS's matches a piece of
 * the name ("." matches anything, as the regex would). Zombies are left out.
 *
 * Uses: process_get_table (core/process), users_find_name (user_management),
 *       strstr-like matching (in this file), arg_next, ui.
 */

#include "modules/process_management/commands/pgrep/cmd_pgrep.h"     // this file
#include "modules/process_management/core/process/process.h"          // process_get_table
#include "modules/user_management/core/users/users.h"                 // users_find_name
#include "lib/args/args.h"                                            // arg_next
#include "lib/string/string.h"                                        // strcmp, strlen, strncmp
#include "modules/ui/core/style/ui.h"                                 // ui_printf, ui_usage

// Does 'name' contain 'piece'? ("." = anything.)
static int contains(const char *name, const char *piece) {
    if (strcmp(piece, ".") == 0)
        return 1;
    int n = strlen(piece);
    for (; *name; name++)
        if (strncmp(name, piece, n) == 0)
            return 1;
    return 0;
}

void cmd_pgrep(const char *args) {
    char word[PROCESS_NAME_LEN], pattern[PROCESS_NAME_LEN] = "";
    int long_form = 0, exact = 0;
    int32_t uid = -1;

    for (const char *rest = arg_next(args, word, sizeof(word)); word[0];
         rest = arg_next(rest, word, sizeof(word))) {
        if (strcmp(word, "-l") == 0) {
            long_form = 1;
        } else if (strcmp(word, "-x") == 0) {
            exact = 1;
        } else if (strcmp(word, "-u") == 0) {
            rest = arg_next(rest, word, sizeof(word));
            const user_t *u = users_find_name(word);
            if (!u) {
                ui_error("pgrep", "no such user");
                return;
            }
            uid = (int32_t)u->uid;
        } else {
            strncpy(pattern, word, sizeof(pattern) - 1);
        }
    }
    if (!pattern[0]) {
        ui_usage("pgrep [-l] [-x] [-u user] pattern");
        return;
    }

    process_t *table = process_get_table();
    int found = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_t *p = &table[i];
        if (p->state == PROCESS_STATE_FREE || p->state == PROCESS_STATE_ZOMBIE)
            continue;
        if (uid >= 0 && p->uid != (uint32_t)uid)
            continue;
        if (exact ? strcmp(p->name, pattern) != 0 : !contains(p->name, pattern))
            continue;
        ui_printf(long_form ? "%u %s\n" : "%u\n", p->pid, p->name);
        found++;
    }
    if (!found)
        ui_printf("(no process matches)\n");
}
