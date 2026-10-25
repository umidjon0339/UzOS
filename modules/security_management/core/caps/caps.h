#ifndef CAPS_H
#define CAPS_H

/*
 * caps.h - Capabilities: who may do which privileged thing.
 *
 * "Is this user root?" is a blunt question. A capability is one
 * privilege with a name, such as CAP_KILL: stop other users' processes.
 * Every check in the kernel asks for exactly the privilege it needs:
 *
 *     if (!caps_has(caller_uid, CAP_KILL)) ...      a rule (pure: uid in)
 *     if (!caps_require(CAP_USERS, "useradd")) ...  a command (prints why)
 *
 * Who holds which capability is decided in ONE place, the table in
 * caps.c, not in a dozen `uid == 0` tests spread over the kernel. Today
 * root holds all of them and everybody else none, so nothing changes
 * for the user; but the rules can now be read, listed (`caps`) and
 * changed in one spot. This is "least privilege": ask for the one right
 * you need, not for everything. Linux works the same way (man 7
 * capabilities): CAP_KILL, CAP_SETUID, CAP_CHOWN, CAP_SYS_NICE, ...
 */

#include <stdint.h>

typedef enum {
    CAP_KILL,           // stop processes of other users        kill_guard
    CAP_SETUID,         // become another user without a password  setuid_guard, su
    CAP_FILES,          // ignore file modes and owners          file_access
    CAP_CHOWN,          // give files to another user            chown
    CAP_USERS,          // add, remove, re-password accounts     useradd, userdel, passwd
    CAP_PRIORITY,       // raise priorities, renice others       priority_bounds
    CAP_RESOURCE,       // no process quota, use the reserve     slot_quota
    CAP_SECLAB,         // run live attacks and repair           seclab
    CAP_COUNT
} cap_t;

#define CAP_BIT(cap) (1u << (cap))
#define CAPS_NONE    0u
#define CAPS_ALL     (CAP_BIT(CAP_COUNT) - 1)

// The set of capabilities a user holds (one bit per cap_t). Pure: the
// answer depends only on the uid, so selftests can ask it too.
uint32_t caps_of(uint32_t uid);
int caps_has(uint32_t uid, cap_t cap);

// The same question for the running process (process_caller_uid).
int caps_caller_has(cap_t cap);

// For commands: 1 if the caller holds 'cap'. Otherwise print
// "<who>: needs CAP_X (root has it)", write it to the audit log, and
// return 0.
int caps_require(cap_t cap, const char *who);

const char *caps_name(cap_t cap);       // "CAP_KILL"
const char *caps_about(cap_t cap);      // "stop processes of other users"

#endif
