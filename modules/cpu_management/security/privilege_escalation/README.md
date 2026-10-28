# privilege_escalation -> permission_check

**Attack.** Take an action only a more privileged user should: lower
your nice below your budget, or make yourself real-time, and you outrank
everyone. The numbers are legal; a user just is not allowed to use them.

**Defence (permission_check).** Every privileged CPU syscall
(`setpriority`, `sched_setscheduler`, `sched_setattr`) checks the
caller's uid and caps through `def_nice_change_allowed` /
`def_policy_change_allowed` (security/priority_abuse). A user gets only
what its rlimits allow; root (CAP_PRIORITY) gets the rest. This is the
partner of priority_abuse: that one sends bad numbers, this one sends
legal numbers a user may not use.
