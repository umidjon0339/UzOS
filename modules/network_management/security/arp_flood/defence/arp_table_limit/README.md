# Defence: arp_table_limit

| | |
|---|---|
| **Stops** | [`arp_flood`](../../) |
| **Protects** | resource: the ARP cache has room for the neighbours we really talk to |
| **Called from** | `arp_receive()` in [core/arp/arp.c](../../../../core/arp/arp.c) |

## The rule

At most **8** cache entries may come from requests we did not ask for.

## Why

Entries we asked for are the ones we need. Strangers may ask, and get
an answer, but they do not get to fill the table.

## Try it

```
seclab verify arp_table_limit
seclab attack arp_flood live
```
