# Defence: kill guard

**Counters:** [`kill_init_bypass`](../../attack/kill_init_bypass/README.md) · **Restores:** `privilege`

## The idea

A kill is decided where the kill happens, not in the shell command that
asks for it. `def_kill_allowed()` is called from `process_kill()`, so
every caller is filtered the same way: the shell, a syscall, a driver,
another module. Moving the old `pid == 1` check out of `cmd_kill()` and
into the kernel is the entire fix.

Two rules:

| Rule | Meaning | Why |
|---|---|---|
| protected PIDs | PID 0 means "no process", PID 1 is the system | nothing may destroy the process the OS stands on |
| kill ownership | a non-root caller may only kill its own processes | one user must not end another user's work |

Killing yourself is allowed by the policy: a process ending itself is
normal. `def_self_kill_policy()` only decides *how* to free that slot
safely, later.

## The policy is a pure function

```c
static int kill_allows(uint32_t caller_pid, uint32_t caller_uid,
                       uint32_t target_pid, uint32_t target_uid);
```

Four numbers in, one verdict out. No table lookups, no logging, no side
effects - so `selftest()` can walk the boundary cases (root, own PID,
foreign PID, PID 0) without creating a single process.

The table lookups and the `sec_log()` call live in `def_kill_allowed()`
instead. That is the split: keep the decision pure, keep the side
effects in one place.

## Where the kernel calls it

```c
/* modules/process_management/core/process.c */
if (!def_kill_allowed(caller_pid, pid)) return;
```

Delete that line and the guard becomes dead code: the fork-bomb style
bypass is back, even though `seclab verify kill_guard` still passes,
because the unit test only checks `kill_allows()`. The real proof is
the event log - `seclab log` shows `kill_guard` refusals only while the
call is live.

## Try it

```
seclab verify kill_guard            # BLOCKED = boundary cases hold
seclab attack kill_init_bypass live # direct process_kill(1)
seclab log                          # evidence the guard actually ran
```
