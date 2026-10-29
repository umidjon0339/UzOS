# signal_abuse -> signal_permission

**Attack.** Send a signal (SIGINT, SIGKILL, ...) to a process you don't
own, or to one of the kernel's processes, to stop or disturb it.

**Defence (signal_permission).** The signal-sending paths ask
`def_signal_allowed` first: a system process is never signalled, a
CAP_KILL holder may signal any other, and otherwise you may signal only
your own processes. (`kill_guard` is the same idea for the `kill`
command; this is the general rule for every signal path.)
