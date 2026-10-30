# Attack: sockbuf_exhaustion

| | |
|---|---|
| **Stopped by** | [`rmem_limit`](defence/rmem_limit/) |
| **Breaks** | resource: one socket cannot hold every packet buffer |
| **Severity** | medium |

## What it does

A receiver that never reads holds kernel memory; counted by payload alone, tiny datagrams eat the whole buffer pool.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains truesize |
| `live` | sends 100 one-byte datagrams over lo to a socket that never reads |

The socket keeps 16; the rest are dropped and the pool stays free.

## Try it

```
seclab attack sockbuf_exhaustion live
seclab verify rmem_limit

```
