# Attack: vlan_hopping (CONCEPT)

| | |
|---|---|
| **Stopped by** | [`vlan_pruning`](defence/vlan_pruning/) |
| **Breaks** | isolation: a VLAN's traffic stays in that VLAN |
| **Severity** | medium |

## What it does

Double tagging or switch spoofing lets a host send frames into a VLAN it is not in.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the attack |
| `live` | nothing: it is a concept |

## Try it

```
seclab attack vlan_hopping
seclab verify vlan_pruning

```
