# Attack: dhcp_starvation (CONCEPT)

| | |
|---|---|
| **Stopped by** | [`dhcp_snooping`](defence/dhcp_snooping/) |
| **Breaks** | resource: the DHCP server has addresses for every real machine |
| **Severity** | medium |

## What it does

DHCP starvation (Yersinia, gobbler) empties a server's address pool with fake clients.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the attack |
| `live` | nothing: it is a concept |

## Try it

```
seclab attack dhcp_starvation
seclab verify dhcp_snooping

```
