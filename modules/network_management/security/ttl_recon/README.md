# Attack: ttl_recon (ILLUSTRATION)

| | |
|---|---|
| **Stopped by** | [`ttl_normalize`](defence/ttl_normalize/) |
| **Breaks** | disclosure: our packets say as little as possible about what we run |
| **Severity** | info |

## What it does

Initial TTL (and window size, options order) fingerprint an OS without logging in.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | lists common initial TTLs |
| `live` | pings 127.0.0.1 and names the OS its TTL suggests |

UzOS answers with 64: indistinguishable from Linux by TTL.

## Try it

```
seclab attack ttl_recon live
seclab verify ttl_normalize

```
