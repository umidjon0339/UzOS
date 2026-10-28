# scheduling_manipulation -> policy_restriction

**Attack.** Set your own policy to SCHED_FIFO and you run before every
normal process - jumping the queue with no busy loop.

**Defence (policy_restriction).** `sched_setscheduler`/`sched_setattr`
ask `def_policy_change_allowed` (security/priority_abuse): a user may
pick a normal policy for its own process, but a real-time one only with
RLIMIT_RTPRIO, and never for a system process. Root may do anything in
range.
