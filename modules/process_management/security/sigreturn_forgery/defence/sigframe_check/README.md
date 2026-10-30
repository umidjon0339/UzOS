# Defence: sigframe_check

| | |
|---|---|
| **Stops** | [`sigreturn_forgery`](../../) |
| **Protects** | privilege: no ring 3 program gets IOPL, VM or a kernel EIP |
| **Called from** | `sys_rt_sigreturn()` in [syscalls/rt_sigreturn](../../../../syscalls/rt_sigreturn/) |

## The rule

1. The frame must be the **newest one the kernel pushed** for this
   process (`process_t.sig_frame[]`); each comes back once.
2. The saved EIP and ESP must be user-space addresses.
3. EFLAGS keeps only what a program may set itself (arithmetic flags,
   DF, TF, AC, RF): **IOPL, NT and VM are cleared, IF is set**.

A refused frame ends the program with SIGSEGV.

## Why

Rule 1 refuses a frame that no signal made (the classic SROP start).
A real frame is still in the program's memory, so the handler can edit
it; rule 3 makes that harmless. Linux does rule 3 (`FIX_EFLAGS`); rule 1
is UzOS's extra check.

## Try it

```
seclab verify sigframe_check
seclab attack sigreturn_forgery live
```
