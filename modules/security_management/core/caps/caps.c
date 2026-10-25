/*
 * caps.c - The capability table, and the checks that read it.
 *
 * Uses: UID_ROOT (user_management), process_caller_uid
 *       (process_management), audit_log (core/audit), ui_* (ui module).
 */

#include "caps.h"
#include "audit.h"
#include "users.h"
#include "process.h"
#include "ui.h"

/* ---------- Who holds what ----------
 * The whole policy. Root holds every capability; every other user none.
 * To give a user one right, it would be added here, nowhere else. */

uint32_t caps_of(uint32_t uid) {
    return uid == UID_ROOT ? CAPS_ALL : CAPS_NONE;
}

/* ---------- Names ---------- */

static const struct {
    const char *name;
    const char *about;
    const char *refused;        // the audit log's text when it is missing
} info[CAP_COUNT] = {
    [CAP_KILL]     = { "CAP_KILL",     "stop processes of other users",    "refused: needs CAP_KILL"     },
    [CAP_SETUID]   = { "CAP_SETUID",   "become another user, no password", "refused: needs CAP_SETUID"   },
    [CAP_FILES]    = { "CAP_FILES",    "ignore file modes and owners",     "refused: needs CAP_FILES"    },
    [CAP_CHOWN]    = { "CAP_CHOWN",    "give files to another user",       "refused: needs CAP_CHOWN"    },
    [CAP_USERS]    = { "CAP_USERS",    "add, remove, re-password users",   "refused: needs CAP_USERS"    },
    [CAP_PRIORITY] = { "CAP_PRIORITY", "raise priorities, renice others",  "refused: needs CAP_PRIORITY" },
    [CAP_RESOURCE] = { "CAP_RESOURCE", "no process quota, use the reserve", "refused: needs CAP_RESOURCE" },
    [CAP_SECLAB]   = { "CAP_SECLAB",   "run live attacks and repair",      "refused: needs CAP_SECLAB"   },
};

static int valid(cap_t cap) {
    return (unsigned)cap < CAP_COUNT;
}

const char *caps_name(cap_t cap)  { return valid(cap) ? info[cap].name  : "?"; }
const char *caps_about(cap_t cap) { return valid(cap) ? info[cap].about : "?"; }

/* ---------- Checks ---------- */

int caps_has(uint32_t uid, cap_t cap) {
    return valid(cap) && (caps_of(uid) & CAP_BIT(cap)) != 0;
}

int caps_caller_has(cap_t cap) {
    return caps_has(process_caller_uid(), cap);
}

int caps_require(cap_t cap, const char *who) {
    if (caps_caller_has(cap))
        return 1;

    audit_log("caps", valid(cap) ? info[cap].refused : "refused: unknown capability");
    ui_start(UI_ERROR, who);
    ui_printf("needs %s (root has it): type 'su' first", caps_name(cap));
    ui_end();
    return 0;
}
