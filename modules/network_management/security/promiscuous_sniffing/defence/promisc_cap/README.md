# Defence: promisc_cap

| | |
|---|---|
| **Stops** | [`promiscuous_sniffing`](../../) |
| **Protects** | disclosure: a user sees only the traffic meant for this machine |
| **Called from** | `netdev_set_promisc()` in [core/netdev/netdev.c](../../../../core/netdev/netdev.c) |

## The rule

Turning promiscuous mode on or off needs **CAP_NET_RAW**.

## Why

It shows other machines' frames: a privilege, like raw sockets.

## Try it

```
seclab verify promisc_cap
seclab attack promiscuous_sniffing live
```
