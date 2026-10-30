# Defence: dns_txid_port_random

| | |
|---|---|
| **Stops** | [`dns_spoofing`](../../) |
| **Protects** | trust: only the real DNS server's answer is believed |
| **Called from** | `dns_query()` in [core/dns/dns.c](../../../../core/dns/dns.c) |

## The rule

Random id, random source port; the answer must match both and the question.

## Why

RFC 5452 (Kaminsky's attack, 2008).

## Try it

```
seclab verify dns_txid_port_random
seclab attack dns_spoofing live
```
