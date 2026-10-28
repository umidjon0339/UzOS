# cpu_management/security — attacks and their defences

Every pair lives in its own folder named for the **attack**, and the
**defence** that stops it lives *inside* that folder:

```
security/<attack>/atk_<attack>.c   (+ a ring 3 .s program where needed)
security/<attack>/README.md
security/<attack>/defence/<defence>/def_<defence>.c / .h
security/<attack>/defence/<defence>/README.md
```

`seclab list` shows them all; `seclab attack <id>` simulates one,
`seclab attack <id> live` runs it, `seclab verify <defence>|all` runs the
defence selftests. A defence is the exact check the kernel already makes
at the point the attack strikes — the folder owns the rule, a selftest,
and a README; it never holds its own copy of kernel state.

## Real demos — the defence is live kernel code

| Attack | Defence | What the kernel does |
|---|---|---|
| `starvation` | `aging` | a waiting process rises one level per 100 ms |
| `priority_abuse` | `priority_bounds` | nice/policy values are range- and permission-checked |
| `side_channel` | `tsc_restrict` | ring 3 `rdtsc` is trapped (CR4.TSD) |
| `user_exception` | `fault_isolation` | a ring 3 fault kills only that process |
| `cpu_exhaustion` | `cpu_time_limit` | RLIMIT_CPU: the timer kills a CPU hog |
| `infinite_loop` | `cpu_watchdog` | same CPU-time budget stops a spin |
| `resource_exhaustion` | `resource_quota` | every finite resource carries an rlimit |
| `fork_bomb` (`cpu_fork_bomb`) | `process_quota` | RLIMIT_NPROC caps processes per user |
| `denial_of_service` | `fair_scheduling` | aging + round robin always serve the waiter |
| `scheduling_manipulation` | `policy_restriction` | a user cannot take a real-time policy freely |
| `realtime_scheduling_abuse` | `rt_throttling` | real-time work gets at most 95% of a second |
| `privilege_escalation` | `permission_check` | every privileged CPU call checks uid and caps |

## Illustrations — a real attack's boundary, without a working exploit

UzOS is a simple 32-bit kernel; these cannot be weaponised here. The
attack explains the technique and checks the boundary it targets; the
defence is that real boundary.

| Attack | Defence | Boundary shown |
|---|---|---|
| `spectre` | `user_kernel_isolation` | kernel pages have no user bit |
| `meltdown` | `page_separation` | ring 3 cannot read kernel pages |
| `cache_timing` | `coarse_timer` | no fine clock in ring 3 (rdtsc trapped) |
| `branch_prediction` | `speculation_barrier` | isolation bounds any leak |

## Concepts — documented where UzOS cannot reproduce them

No attack code runs; the README and defence explain the real fix and why
it lives outside a teaching guest kernel.

| Attack | Defence | Why it is a concept |
|---|---|---|
| `distributed_denial_of_service` | `rate_limiting` | no network stack |
| `rowhammer` | `ecc_and_refresh` | emulated RAM does not flip bits |
| `return_oriented_programming` | `wx_and_aslr` | 32-bit paging has no NX bit |
| `code_injection` | `nx_pages` | 32-bit paging has no NX bit |
| `hypervisor_escape` | `vm_isolation` | UzOS is the guest; escape attacks the host |

The pairs are registered in
[security_management/core/catalog/catalog.c](../../security_management/core/catalog/catalog.c);
the shared `attack_t` / `defence_t` types are in
[security_management/core/registry](../../security_management/core/registry/).
