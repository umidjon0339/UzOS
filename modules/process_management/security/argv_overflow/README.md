# Attack: argv_overflow

| | |
|---|---|
| **Stopped by** | [`arg_limits`](defence/arg_limits/) |
| **Breaks** | resource: execve copies argv/envp into the kernel and onto the new stack |
| **Severity** | medium |

## What it does

A huge argv / envp overflows execve's kernel buffer, or fills the new
program's stack before `main()`. An argument pointer into the kernel
makes execve copy kernel memory into the new program.

## In UzOS

`live` hands `arg_limits` 100 arguments, one 3000-byte argument, and an
argument pointing at 0x00100000 (as from ring 3): all refused.
`proccalls exec` does the same with a real `execve()` from ring 3.

## Try it

```
seclab attack argv_overflow live
proccalls exec
```
