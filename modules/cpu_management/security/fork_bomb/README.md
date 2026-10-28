# fork_bomb -> process_quota

**Attack.** `while (1) fork();` doubles processes until the table is
full and nothing - not even root - can start.

**Defence (process_quota).** RLIMIT_NPROC (`ulimit -u`) caps how many
processes one *user* may own at once; `process_create` asks
`rlimit_nproc_ok()` and refuses the fork past the cap. Root keeps a
reserve (CAP_RESOURCE) so an admin can always clean up.

This is the per-user view. The total process-table guard lives in
process_management (slot_quota). To avoid a clash, this attack's seclab
id is **`cpu_fork_bomb`**.
