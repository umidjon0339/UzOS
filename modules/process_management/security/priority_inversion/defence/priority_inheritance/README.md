# Defence: priority_inheritance (concept)

| | |
|---|---|
| **Stops** | [`priority_inversion`](../../) |
| **Protects** | liveness: priorities stay the right way up |
| **Called from** | nothing yet: UzOS has no shared locks (no futex) |

## The rule

While a process waits for a lock, the lock's holder runs at the
waiter's priority (`def_pi_priority`: the higher of the two) until it
lets go. Linux: PI futexes, `rt_mutex`.
