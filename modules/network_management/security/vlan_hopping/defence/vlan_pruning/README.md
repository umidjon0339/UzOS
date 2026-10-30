# Defence: vlan_pruning (CONCEPT)

| | |
|---|---|
| **Stops** | [`vlan_hopping`](../../) |
| **Protects** | isolation: a VLAN's traffic stays in that VLAN |
| **Called from** | nothing in UzOS: trunks are pruned to the vlans they need; no native vlan; dtp off on access ports |

## The rule

Trunks are pruned to the VLANs they need; no native VLAN; DTP off on access ports.

## Why

Every hop technique needs a trunk or the native VLAN.

## Try it

```
seclab verify vlan_pruning
seclab attack vlan_hopping
```
