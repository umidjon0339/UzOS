# Attack: PID wrap

**Breaks:** `identity` · **Severity:** HIGH · **Paired defence:** [`pid_bounds`](../../defence/pid_bounds/README.md)

## The idea

A PID must name exactly one process. On UzOS, PIDs come from a counter
that only ever goes up. Nothing stopped it reaching `0xFFFFFFFF` and
wrapping back to `0`, `1`, `2` ... - numbers that may still be in use.

Two live processes would then share a PID. `process_get_by_pid()` has
to pick one, the other becomes invisible, and `kill 7` can hit a
process you never meant to touch.

This breaks *identity*, not *resource*: the table is not full, it lies.

## How it works here

The real wrap needs about four billion creations (`2^32` =
`4,294,967,296`). No demo can sit through that, so this module does
not fake it:

- `simulate()` reads the process table and does the arithmetic. It
  never calls the guard, because the guard writes to the event log, so
  the run stays strictly read-only.
- `launch()` creates no process and kills none. It asks
  `def_pid_bounds_check()` about the boundary PIDs a wrap would
  produce: `0xFFFFFF00`, `0xFFFFFFFF` and `0`, with `1` and
  `0xFFFFFFFE` as the healthy cases. Three of the five are refused.
- `cleanup()` is `0`, because a run leaves nothing behind to undo.

`0xFFFFFF00` is the defence's own `PID_LIMIT`. The distance from there
to `2^32` is `0x100` = **256 PIDs** that are deliberately never handed
out - the price of never wrapping.

## Try it

```
seclab attack pid_wrap          # safe: shows the arithmetic
seclab attack pid_wrap live     # probes the guard at the boundary
seclab verify pid_bounds        # the guard's own selftest
seclab log                      # the refusals, as evidence
```

## What you should see

A live run probes five PIDs and reports three refusals, so the verdict
is BLOCKED and the log gains `pid_bounds` entries. Nothing is created,
nothing is killed, and `seclab status` shows the same free slots as
before.

The lesson is the arithmetic, not the exploit: a counter that only goes
up still collides once it wraps, so it needs a ceiling.
