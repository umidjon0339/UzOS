#ifndef DEF_POLICY_RESTRICTION_H
#define DEF_POLICY_RESTRICTION_H

// 1 if changing scheduling policy is restricted correctly: a user may
// pick a normal policy for its own process, but not a real-time one
// without RLIMIT_RTPRIO, and never touch a system process. The rule is
// def_policy_change_allowed (security/priority_abuse), called by
// sched_setscheduler / sched_setattr; this reports that it holds.
int def_policy_restriction_ok(void);

#endif
