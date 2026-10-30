# Attack: syscall_abuse

| | |
|---|---|
| **Stopped by** | [`seccomp_filter`](defence/seccomp_filter/) |
| **Breaks** | privilege: a sandboxed program keeps only the syscalls it kept |
| **Severity** | high |

## What it does

A program that reads untrusted input can be taken over by it; the
attacker's code then has every syscall the program has. If the program
put itself in a seccomp sandbox first, the attacker gets only the few
calls it really needs.

## In UzOS

[syscall_abuse.s](syscall_abuse.s) runs in ring 3 as uid 1: it sets
`no_new_privs`, installs a filter (exit, getpid, write, rt_sigreturn,
seccomp; others fail), then "the attacker" calls `open`, `kill(1)`,
`setuid(0)`: all -1. Last it goes `seccomp(STRICT)` and calls
`getpid()`: the program ends with SIGSYS.

## Try it

```
seclab attack syscall_abuse live
audit
```
