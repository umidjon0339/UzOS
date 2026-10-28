# cpu_exhaustion -> cpu_time_limit

**Attack.** A process that never sleeps uses a whole CPU second every
second. A runaway or hostile loop keeps the CPU from everything else.

**Defence (cpu_time_limit).** RLIMIT_CPU (`ulimit -t N`) gives a process
N CPU seconds; the timer tick checks the total through
`rlimit_check_cpu()` and kills it once it is over. A child inherits the
limit, so setting it in the shell caps every command started afterwards.

Try it: `ulimit -t 1`, then run a busy command; it dies after ~1 s.
