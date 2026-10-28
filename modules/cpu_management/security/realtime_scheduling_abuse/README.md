# realtime_scheduling_abuse -> rt_throttling

**Attack.** A SCHED_FIFO task runs before every normal task and need
never yield, so a real-time busy loop would own the CPU for good.

**Defence (rt_throttling).** Two gates. You need RLIMIT_RTPRIO to become
real-time at all (see policy_restriction). Even then, the scheduler
caps all real-time work at 95% of each second (`priority_tick`); past
the budget it runs normal tasks, so the shell survives.
