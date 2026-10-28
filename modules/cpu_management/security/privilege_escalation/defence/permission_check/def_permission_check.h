#ifndef DEF_PERMISSION_CHECK_H
#define DEF_PERMISSION_CHECK_H

// 1 if the CPU module refuses a non-root process the privileged actions
// it must not have: lowering nice below its budget, and taking a
// real-time policy without RLIMIT_RTPRIO. The rules are
// def_nice_change_allowed / def_policy_change_allowed
// (security/priority_abuse); this reports that they hold for a user and
// yield for root.
int def_permission_check_ok(void);

#endif
