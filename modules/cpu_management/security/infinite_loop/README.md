# infinite_loop -> cpu_watchdog

**Attack.** `while (1);` makes no progress but never gives the CPU back.

**Defence (cpu_watchdog).** A kernel cannot tell a stuck loop from
useful work, so the "watchdog" is a CPU-time budget: the timer tick
(`rlimit_check_cpu`) kills a process that runs past its RLIMIT_CPU. Same
machinery as cpu_exhaustion, different lesson: you do not detect "stuck",
you bound how long anything may run.
