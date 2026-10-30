# Defence: frag_mem_limit

| | |
|---|---|
| **Stops** | [`fragment_flood`](../../) |
| **Protects** | resource: the reassembly slots are shared fairly |
| **Called from** | `ipv4_reassemble()` in [core/ipv4/fragment.c](../../../../core/ipv4/fragment.c) |

## The rule

At most 2 of the 4 slots per source address; 30 s timeout.

## Why

Shared memory needs a share per user.

## Try it

```
seclab verify frag_mem_limit
seclab attack fragment_flood live
```
