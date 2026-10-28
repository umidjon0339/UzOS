# resource_exhaustion -> resource_quota

**Attack.** Grab every finite resource - process slots, file
descriptors, heap - not only CPU time.

**Defence (resource_quota).** Each resource carries a per-process limit
(rlimit), checked where it is handed out: CPU in the timer, NPROC in
`process_create`, NOFILE in `open`, FSIZE in `write`, DATA in `sbrk`.
A child inherits the limits, so one quota covers a whole process tree.
