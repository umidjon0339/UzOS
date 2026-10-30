# Defence: dhcp_validate

| | |
|---|---|
| **Stops** | [`dhcp_spoofing`](../../) |
| **Protects** | trust: our address, router and DNS come from the real DHCP server |
| **Called from** | `on_message()` in [core/dhcp/dhcp.c](../../../../core/dhcp/dhcp.c) |

## The rule

An offer names its server, its router is on the offered network, and after the first lease it comes from the same server.

## Why

DHCP cannot prove who speaks; the client can at least stay with the server it knows.

## Try it

```
seclab verify dhcp_validate
seclab attack dhcp_spoofing live
```
