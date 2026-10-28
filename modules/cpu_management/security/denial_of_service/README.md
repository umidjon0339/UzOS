# denial_of_service -> fair_scheduling

**Attack.** Keep the CPU so busy that the shell and servers never run -
a denial of service from inside one machine.

**Defence (fair_scheduling).** Aging (security/starvation) raises a
waiting process one level per 100 ms until it ties the busiest task;
the round robin then gives it a turn. The scheduler calls this on every
decision, so the longest waiter is always served in the end.
