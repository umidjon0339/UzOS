# process_management/security — attacks and their defences

Every pair lives in its own folder named for the **attack**, with the
**defence** nested inside it (`security/<attack>/defence/<defence>/`), the
same clean shape as cpu_management and device_management.

`seclab list` shows them all; `seclab attack <id> [live]` runs one;
`seclab verify <defence>|all` runs the selftests.

## Real — the defence is live kernel code

| Attack | Defence | Kernel caller |
|---|---|---|
| `fork_bomb` | `slot_quota` | `process_create` caps processes per user |
| `pid_wrap` | `pid_bounds` | PID allocation stays in range / never reuses a live PID |
| `kill_system` | `kill_guard` | the `kill` path refuses a process you may not kill |
| `zombie_leak` | `reaper` | the scheduler's reaper frees every zombie |
| `stack_overflow` | `stack_guard` | the scheduler checks a stack has not overrun |
| `uninit_pcb` | `pcb_wipe` | a reused slot is wiped before use |
| `kernel_stack_to_user` | `user_stack` | a ring-3 task runs only on a user stack |
| `malicious_elf` | `elf_validate` | the loader checks an ELF before running it |
| `signal_abuse` | `signal_permission` | a signal reaches only your own processes |
| `orphan_process` | `orphan_reaping` | the global reaper frees orphans too |

## Rule shown now, wired when the syscall lands

| Attack | Defence | Note |
|---|---|---|
| `ptrace_abuse` | `ptrace_restrict` | the ownership gate a future `ptrace()` would need |

## Illustrations / concepts (UzOS is a simple single-address-space kernel)

| Attack | Defence | Why |
|---|---|---|
| `toctou_race` | `atomic_check` | illustrates the check-then-use gap and the one-step fix |
| `process_injection` | `process_isolation` | full isolation needs a per-process address space |
| `process_hijacking` | `control_flow_integrity` | real CFI is a compiler + hardware feature |
| `container_escape` | `namespace_isolation` | UzOS has no namespaces/containers |
| `process_dos` | `service_limits` | umbrella; the real caps are slot_quota + CPU quotas |

Registered in
[security_management/core/catalog/catalog.c](../../security_management/core/catalog/catalog.c).
