# Attack: pid_reuse_race

| | |
|---|---|
| **Stopped by** | [`pidfd_handle`](defence/pidfd_handle/) (and [`pcb_generation`](../use_after_exit/defence/pcb_generation/) inside it) |
| **Breaks** | identity: "that process" must not turn into another one |
| **Severity** | high |

## What it does

`pgrep worker` → `1234` ... the worker ends ... `kill 1234`. Where PIDs
wrap around (Linux, `pid_max`), 1234 may now be a different process,
and it gets the signal. UzOS never reuses a PID (`pid_bounds`), but it
does reuse process **slots**: a handle that only remembered the slot
would have the same race.

## In UzOS

`live` (as root): `pidfd_open(race_victim)`, kill and reap it, start
`race_new` until it gets the same slot, then
`pidfd_send_signal(old pidfd, SIGSTOP)`: it must fail and `race_new`
must keep running. Last, user 1 asks for `pidfd_getfd` on a root
process: refused.

## Try it

```
seclab attack pid_reuse_race live
seclab repair
```
