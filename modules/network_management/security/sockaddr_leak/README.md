# Attack: sockaddr_leak

| | |
|---|---|
| **Stopped by** | [`sockaddr_zero`](defence/sockaddr_zero/) |
| **Breaks** | disclosure: an address the kernel hands out carries no leftover kernel memory |
| **Severity** | medium |

## What it does

Kernel infoleaks through uninitialised padding of structures copied to user space.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the padding |
| `live` | getsockname() into a buffer of 0xAA; checks sin_zero |

The padding comes back all zero.

## Try it

```
seclab attack sockaddr_leak live
seclab verify sockaddr_zero

```
