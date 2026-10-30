# Attack: fd_leak_on_exec

| | |
|---|---|
| **Stopped by** | [`close_on_exec`](defence/close_on_exec/) |
| **Breaks** | disclosure: an open file belongs to the program that opened it |
| **Severity** | medium |

## What it does

`execve` keeps the descriptor table. A privileged program that opened a
secret and then runs a helper hands the helper that open file: no
permission check ever happens.

## In UzOS

`live`: a stand-in process holds `secret` (O_CLOEXEC) and `log` (no
flag); `close_on_exec` runs as at execve: the secret is closed, the log
stays. `proccalls exec` checks it with a real execve: the new program's
`read(3)` fails.

## Try it

```
seclab attack fd_leak_on_exec live
proccalls exec
```
