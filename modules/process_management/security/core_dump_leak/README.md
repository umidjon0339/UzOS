# Attack: core_dump_leak

| | |
|---|---|
| **Stopped by** | [`dumpable_flag`](defence/dumpable_flag/) |
| **Breaks** | disclosure: secrets read as root stay root's |
| **Severity** | medium |

## What it does

A daemon starts as root, reads its private key, then drops to a normal
user with `setuid()`. The key is still in its memory, and the process
now belongs to that user, who can read it with a debugger,
`process_vm_readv`, or (on Linux) the core file of a crash.

## In UzOS

`live`: `daemon` (root) does `setuid(1)` the way `sys_setuid` does, so
`dumpable_flag` makes it non-dumpable. Then user 1's `thief` asks to
read its memory (`vm_access_check`): refused. UzOS writes no core
files; the flag guards every other way in.

## Try it

```
seclab attack core_dump_leak live
seclab repair
```
