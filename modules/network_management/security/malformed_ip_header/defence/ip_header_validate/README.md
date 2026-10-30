# Defence: ip_header_validate

| | |
|---|---|
| **Stops** | [`malformed_ip_header`](../../) |
| **Protects** | trust: the IP header's lengths fit the packet that carries them |
| **Called from** | `ipv4_receive()` in [core/ipv4/ipv4.c](../../../../core/ipv4/ipv4.c) |

## The rule

Version 4, 20 <= header <= total <= received, checksum 0.

## Why

Check every length before it is used.

## Try it

```
seclab verify ip_header_validate
seclab attack malformed_ip_header live
```
