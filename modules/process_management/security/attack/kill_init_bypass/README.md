# Attack: kill init bypass

**Breaks:** `privilege` · **Severity:** CRITICAL · **Paired defence:** [`kill_guard`](../../defence/kill_guard/README.md)

## The idea

PID 1 is special: it is the process the whole system leans on. The
shell knows that, so `cmd_kill()` refuses `kill 1` before it ever calls
the kernel.

That check guards exactly one door. `process_kill()` is a public kernel
function, and a syscall, a driver or another module can call it
directly - no shell in the way, no check. A guard that lives in the
user interface protects the interface, not the operation.

## How it works here

The attack does what a rogue caller would do: it skips the shell and
calls `process_kill(1)` itself, `ATTACK_BUDGET` (4) times.

`simulate()` only reads the process table. It prints who is calling,
whether PID 1 is alive, and how many live processes a direct call could
reach. It kills nothing.

`launch()` makes the real calls, so the bypass is genuine and not just
described. It stays bounded - a fixed number of attempts - and it never
aims at the process that is currently running, which is an absolute
rule. With `kill_guard` in place every attempt is refused inside
`process_kill()`, so nothing on the machine dies.

There is no `cleanup()`: the attack never changes the table. And if a
kill did get through, no cleanup could bring a dead process back.

## Try it

```
seclab info kill_init_bypass        # what it does, and its defence
seclab attack kill_init_bypass      # safe: what a direct call reaches
seclab attack kill_init_bypass live # real direct process_kill() calls
seclab log                          # the kill_guard refusals
seclab verify kill_guard
```

## What you should see

The live run reports 4 calls and 4 refusals, and the verdict reads
`BLOCKED by the defence`. The event log gains `kill_guard` entries:
evidence that the check ran deep in the kernel, past the shell.

If the `def_kill_allowed()` call were deleted from `process_kill()`,
the first attempt would mark PID 1 as a zombie. Same command, same
caller, no guard - that is the whole lesson.

## Where the defence hooks in

`process_kill()` calls `def_kill_allowed(caller, target)` before it
touches any process. See [`def_kill_guard.c`](../../defence/kill_guard/def_kill_guard.c).
