# Attack: arp_flood

| | |
|---|---|
| **Stopped by** | [`arp_table_limit`](defence/arp_table_limit/) |
| **Breaks** | resource: the ARP cache has room for the neighbours we really talk to |
| **Severity** | medium |

## What it does

Every request aimed at us makes us remember the asker. Floods of them
from invented addresses fill the ARP cache and push the real
neighbours out (or, on a switch, the CAM table).

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | counts the free ARP entries |
| `live` | injects 40 requests for our address from 10.0.2.100-139, each with its own MAC |
| `repair` | deletes the fake entries |

With `arp_table_limit` only 8 are remembered; all are still answered.

## Try it

```
seclab attack arp_flood live
seclab verify arp_table_limit
arp              # Entries: ...
audit
```
