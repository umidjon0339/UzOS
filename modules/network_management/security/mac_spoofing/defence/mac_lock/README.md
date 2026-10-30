# Defence: mac_lock

| | |
|---|---|
| **Stops** | [`mac_spoofing`](../../) |
| **Protects** | privilege: only the administrator decides the card's MAC address |
| **Called from** | `netdev_set_mac()` in [core/netdev/netdev.c](../../../../core/netdev/netdev.c) |

## The rule

Changing a MAC needs **CAP_NET_ADMIN**.

## Why

The MAC is the card's identity on the link.

## Try it

```
seclab verify mac_lock
seclab attack mac_spoofing live
```
