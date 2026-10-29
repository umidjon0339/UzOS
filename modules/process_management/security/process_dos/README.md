# process_dos -> service_limits  (umbrella)

**Attack.** Deny service through process abuse: spawn endlessly, fill the
process table, or hog the CPU so real work never runs.

**Defence (service_limits).** The answer is limits, which already exist
as live defences: per-user process caps (`fork_bomb`/`slot_quota`) and
CPU quotas (`cpu_exhaustion`/`starvation` in cpu_management). This pair
names the umbrella and points at the real enforcement.
