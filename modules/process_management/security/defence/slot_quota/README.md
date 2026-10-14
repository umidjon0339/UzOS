# Defence: slot quota

**Counters:** [`fork_bomb`](../../attack/fork_bomb/README.md) · **Restores:** `resource`

## The idea

Bounding how many processes one user can own, and keeping a small
reserve of slots that only root may touch.

Two rules, checked before any slot is handed out:

| Rule | Meaning | Why |
|---|---|---|
| per-user cap | an unprivileged user holds at most `MAX_PROC_PER_UID` (8) processes | stops one user consuming the table |
| root reserve | an unprivileged user never takes the last `RESERVED_FOR_ROOT` (5) slots | keeps the machine recoverable |

The reserve is the more interesting rule. A completely full process
table cannot be repaired from inside the system, so leaving a little
room for the administrator converts a permanent denial of service into
a temporary one.

This is the same idea as `RLIMIT_NPROC` on Unix, and as the reserved
slots for root that `process_fork()` already hinted at.

## Why the policy is a separate function

```c
static int quota_allows(int free_slots, int owned_by_uid, uint32_t uid);
```

It depends on three numbers and nothing else. No globals, no locks, no
side effects - so `selftest()` can check the boundary cases directly
instead of having to fill the real process table first.

That single line is the whole design lesson of this defence: **keep the
decision pure and put the side effect somewhere else.**

## Where the kernel calls it

```c
/* modules/process_management/core/process.c */
if (!def_slot_quota_check(uid)) return -1;
```

If this call is ever deleted, the defence becomes dead code and the
fork bomb works again. `seclab verify slot_quota` still passes, because
the unit test only checks the policy - which is exactly why the lab
also records events: `seclab log` showing no `slot_quota` entries after
a live fork bomb is the real proof.

## Try it

```
seclab verify slot_quota    # BLOCKED = the boundary cases hold
seclab attack fork_bomb live
seclab log                   # evidence the guard actually ran
```
