# Attack: sigreturn_forgery (SROP)

| | |
|---|---|
| **Stopped by** | [`sigframe_check`](defence/sigframe_check/) |
| **Breaks** | privilege: ring 3 must never get I/O privilege |
| **Severity** | critical |

## What it does

A signal handler returns through `rt_sigreturn`, which loads **every**
register from the frame on the program's own stack. If the kernel
trusts that memory, a program can write any values it likes and call
`rt_sigreturn`: "Sigreturn Oriented Programming". With IOPL = 3 in
EFLAGS, ring 3 may run `cli` and `in`/`out`, so it owns the hardware.

## In UzOS

Two ring 3 programs in [sigreturn_forgery.s](sigreturn_forgery.s), run as uid 1:

| Program | Trick | With sigframe_check |
|---|---|---|
| `srop_forged` | builds a frame by hand, calls `rt_sigreturn` with no signal | refused, ended with SIGSEGV |
| `srop_tamper` | a real SIGUSR1 handler sets IOPL 3 in its own saved EFLAGS | returns, but IOPL is cleared |

`findings` = programs that ran `cli` in ring 3.

## Try it

```
seclab attack sigreturn_forgery live
audit
```
