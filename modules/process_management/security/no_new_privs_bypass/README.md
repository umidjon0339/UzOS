# Attack: no_new_privs_bypass

| | |
|---|---|
| **Stopped by** | [`no_new_privs`](defence/no_new_privs/) |
| **Breaks** | privilege: a filter is never aimed at a stronger program |
| **Severity** | medium |

## What it does

A seccomp filter makes chosen syscalls fail. Inherited by a more
privileged program, "setuid() failed but I carry on as root" is a
classic hole. So a filter needs the `no_new_privs` promise first, and
the promise must stick.

## In UzOS

[no_new_privs_bypass.s](no_new_privs_bypass.s) (ring 3, uid 1) tries a
filter without the promise (refused), sets the promise, tries to take
it back (refused), and checks its child still has it (it does).

## Try it

```
seclab attack no_new_privs_bypass live
```
