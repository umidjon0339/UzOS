# Attack: kernel_pointer_leak

| | |
|---|---|
| **Stopped by** | [`kptr_hide`](defence/kptr_hide/) |
| **Breaks** | disclosure: kernel addresses help an exploit aim |
| **Severity** | low |

## What it does

An exploit needs to know where things are. `procinfo` knows a process's
kernel stack, its saved stack pointer and the kernel function it runs:
handed to a normal user, that is a map for an attack.

## In UzOS

`live` starts `kptr_probe` as uid 1 and asks for its three addresses as
uid 1, the way `procinfo` does. Each must come back as `0`.

## Try it

```
seclab attack kernel_pointer_leak live
procinfo 2              # as root: real addresses
su guest
procinfo 1              # as guest: zeros
```
