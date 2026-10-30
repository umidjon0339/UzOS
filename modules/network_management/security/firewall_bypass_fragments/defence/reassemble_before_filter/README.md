# Defence: reassemble_before_filter

| | |
|---|---|
| **Stops** | [`firewall_bypass_fragments`](../../) |
| **Protects** | trust: a firewall rule sees every packet it is meant to stop |
| **Called from** | `ipv4_receive()` in [core/ipv4/ipv4.c](../../../../core/ipv4/ipv4.c) |

## The rule

Only whole datagrams are filtered.

## Why

RFC 1858; netfilter defragments before filtering.

## Try it

```
seclab verify reassemble_before_filter
seclab attack firewall_bypass_fragments live
```
