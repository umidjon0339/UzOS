# Defence: ttl_normalize (ILLUSTRATION)

| | |
|---|---|
| **Stops** | [`ttl_recon`](../../) |
| **Protects** | disclosure: our packets say as little as possible about what we run |
| **Called from** | `ipv4_send()` in [core/ipv4/ipv4.c](../../../../core/ipv4/ipv4.c) |

## The rule

Default TTL 64.

## Why

You cannot hide the TTL, only make it common.

## Try it

```
seclab verify ttl_normalize
seclab attack ttl_recon live
```
