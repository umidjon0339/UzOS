# Defence: PID bounds

**Counters:** [`pid_wrap`](../../attack/pid_wrap/README.md) · **Restores:** `identity`

## The idea

Give every process a PID no other process holds, and stop the counter
before it wraps. Running out is a clean failure; reusing one is silent.

## The policy

Two rules, both pure arithmetic on the PID alone:

```c
#define PID_LIMIT 0xFFFFFF00u   /* stop well before the counter wraps */

static int pid_in_range(uint32_t pid) {
    if (pid == 0) return 0;         /* 0 means "no process" */
    if (pid >= PID_LIMIT) return 0; /* wrapper would collide */
    return 1;
}
```

| Rule | Meaning | Why |
|---|---|---|
| PID 0 reserved | `pid_in_range(0) == 0` | 0 is the "no process" sentinel |
| ceiling below the wrap | any `pid >= 0xFFFFFF00` is refused | 256 PIDs of slack, so the counter cannot wrap |

The ceiling costs 256 PIDs out of four billion - cheap insurance.

## Why the policy is a separate function

It depends on one number and nothing else: no globals, no locks, no
side effects, so `selftest()` can check the boundaries directly -
`PID_LIMIT`, `PID_LIMIT - 1`, `0`, `0xFFFFFFFF` - instead of running
four billion creations first.

`def_pid_bounds_check()` is the side-effecting wrapper: it asks the
policy and logs a `pid_bounds` event on refusal.

## Where the kernel calls it

```c
/* modules/process_management/core/process.c */
if (!def_pid_bounds_check(next_pid)) return -1;  /* process_create() */
if (!def_pid_bounds_check(pid))      return 0;   /* process_kill()   */
```

The first stops a wrapping PID being handed out. The second stops a
wrapped number being used as a kill target. Delete either and
`pid_wrap` returns.

## Try it

```
seclab verify pid_bounds    # BLOCKED = the boundary cases hold
seclab attack pid_wrap      # the arithmetic, read-only
seclab attack pid_wrap live # three probes refused
seclab log                  # evidence the guard actually ran
```
