# Attack: ptrace_abuse

| | |
|---|---|
| **Stopped by** | [`ptrace_restrict`](defence/ptrace_restrict/) |
| **Breaks** | disclosure: a debugger sees all of a process's memory |
| **Severity** | high |

## What it does

`ptrace()` reads and writes another process's registers and memory.
If any process could attach to any other, it would be a window into
everyone's secrets: keys, passwords, tokens.

## In UzOS

`live` asks ptrace_restrict, as user 1, about user 2's process and a
system process: both refused. `proccalls debug` uses the real
`ptrace()` from ring 3: it may attach to its own child, not to PID 1.

## Try it

```
seclab attack ptrace_abuse live
proccalls debug
strace -p 1          # refused: a system process
```
