/*
 * def_iommu.c - DEFENCE iommu (CONCEPT): an IOMMU limits each device to the memory mapped for it.
 *
 * Stops:  dma_attack (security/dma_attack)
 * Called: nothing in UzOS: needs an IOMMU (VT-d / AMD-Vi).
 * Rule:   a device's DMA goes through its own page table, which holds only its buffers.
 * Why:    page tables protect memory from programs, not from devices.
 *
 * Uses: nothing in UzOS (see the README).
 */

#include "modules/network_management/security/dma_attack/defence/iommu/def_iommu.h" // this file
#include "modules/security_management/core/registry/registry.h"                 // defence_t, registry_add_defence

int def_iommu_holds(void) { return 1; }     // kept outside UzOS: see the README

static int selftest(void) {
    return def_iommu_holds();
}

static const defence_t defence = {
    "iommu",
    "an IOMMU limits each device to the memory mapped for it",
    "dma_attack",
    selftest
};

void def_iommu_register(void) { registry_add_defence(&defence); }
