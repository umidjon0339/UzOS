# Defence: iommu (CONCEPT)

| | |
|---|---|
| **Stops** | [`dma_attack`](../../) |
| **Protects** | isolation: a device writes only the memory its driver gave it |
| **Called from** | nothing in UzOS: an iommu translates device addresses through per-device page tables |

## The rule

An IOMMU translates device addresses through per-device page tables.

## Why

Without it, plugging in a device trusts it with all of RAM.

## Try it

```
seclab verify iommu
seclab attack dma_attack
```
