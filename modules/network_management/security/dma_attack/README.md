# Attack: dma_attack (CONCEPT)

| | |
|---|---|
| **Stopped by** | [`iommu`](defence/iommu/) |
| **Breaks** | isolation: a device writes only the memory its driver gave it |
| **Severity** | high |

## What it does

Bus-master devices access physical memory directly. A malicious device (or firmware) reads secrets or patches the kernel.

## In UzOS

| Mode | What happens |
|---|---|
| `simulate` | explains the attack |
| `live` | nothing: it is a concept |

## Try it

```
seclab attack dma_attack
seclab verify iommu

```
