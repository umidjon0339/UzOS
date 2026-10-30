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
| `ptrace_abuse` | `ptrace_restrict` | `ptrace` ATTACH / TRACEME, `strace -p`: own, dumpable, a descendant (Yama) |
| `signal_flood` | `sigqueue_limit` | `sigqueue_add` keeps 16 queued signals per user |
| `sigreturn_forgery` | `sigframe_check` | `rt_sigreturn` restores only a frame the kernel pushed, never IOPL |
| `handler_hijack` | `handler_bounds` | `rt_sigaction`: SIGKILL/SIGSTOP stay default, handlers in user space |
| `process_masquerade` | `name_guard` | `prctl(PR_SET_NAME)`: printable, never a system process's name |
| `proc_info_leak` | `hidepid` | `procinfo`, `pwdx`, `/proc/<pid>/status`: details for the owner and root only |
| `kernel_pointer_leak` | `kptr_hide` | `procinfo`: kernel addresses print as 0 for normal users |
| `pid_reuse_race` | `pidfd_handle` | a pidfd names one live process; `pidfd_getfd` needs ptrace rights |
| `wait_steal` | `child_only` | only the parent waits for a child and takes its status |
| `use_after_exit` | `pcb_generation` | a saved process reference is checked: same slot, same pid, same life |
| `pgid_hijack` | `session_check` | `setpgid` / `setsid` follow POSIX: groups stay in their session |
| `sleep_overflow` | `sleep_bounds` | every program time becomes ticks in 64 bits, rounded up, capped |
| `timer_flood` | `timer_limits` | at most 8 periodic interval timers per user |
| `syscall_abuse` | `seccomp_filter` | the syscall handler: a sandboxed program gets only the calls it kept |
| `no_new_privs_bypass` | `no_new_privs` | a filter needs the promise; it is sticky and inherited |
| `memory_peek` | `vm_access_check` | `process_vm_readv` / `writev`, ptrace PEEK/POKE: rights, user memory only |
| `argv_overflow` | `arg_limits` | `execve`: 32 strings / 2048 bytes, user pointers only |
| `env_injection` | `env_sanitize` | `execve`: NAME=value, printable, never LD_PRELOAD / IFS / ... |
| `fd_leak_on_exec` | `close_on_exec` | `execve` closes O_CLOEXEC descriptors and pidfds |
| `thread_exhaustion` | `thread_quota` | `clone` / `clone3`: a program has at most 4 threads |
| `stack_clash` | `guard_gap` | `execve`: no segment in the stack area, 32 KiB unmapped below a stack |
| `core_dump_leak` | `dumpable_flag` | `setuid()` makes a process non-dumpable |

## Illustrations / concepts (UzOS is a simple single-address-space kernel)

| Attack | Defence | Why |
|---|---|---|
| `toctou_race` | `atomic_check` | illustrates the check-then-use gap and the one-step fix |
| `process_injection` | `process_isolation` | full isolation needs a per-process address space |
| `process_hijacking` | `control_flow_integrity` | real CFI is a compiler + hardware feature |
| `container_escape` | `namespace_isolation` | UzOS has no namespaces/containers |
| `process_dos` | `service_limits` | umbrella; the real caps are slot_quota + CPU quotas |
| `priority_inversion` | `priority_inheritance` | user programs share no locks (no futex); the rule is tested |
| `tty_hijack` | `tty_guard` | one console, no input-injection call; the rule is tested |

Registered in
[security_management/core/catalog/catalog.c](../../security_management/core/catalog/catalog.c).
