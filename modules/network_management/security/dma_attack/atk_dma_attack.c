/*
 * atk_dma_attack.c - ATTACK dma_attack (CONCEPT): a device that DMAs anywhere (concept)
 *
 * Breaks:     isolation - a device writes only the memory its driver gave it
 * Stopped by: iommu (defence/iommu) - CONCEPT
 *
 * The e1000 writes received frames straight into RAM (bus master). It
 * could write ANY physical address: the CPU's page tables do not apply
 * to devices. Malicious PCIe/Thunderbolt devices do exactly that.
 *
 * Nothing is launched; the defence is described, not run.
 *
 * Uses: def_iommu_holds (its defence), ui_printf.
 */

#include "modules/security_management/core/registry/registry.h"                 // attack_t, registry_add_attack
#include "modules/network_management/security/dma_attack/defence/iommu/def_iommu.h" // def_iommu_holds
#include "modules/ui/core/style/ui.h"                                           // ui_printf, ui_color

static int simulate(void) {
    ui_printf("  the e1000 writes frames into RAM itself (DMA): no CPU, no page tables\n");
    ui_printf("  a malicious device could write any physical address: the kernel too\n");
    ui_printf("  an IOMMU gives devices their own page tables: UzOS (and QEMU here) has none\n");
    return 1;
}

static int launch(void) {
    ui_printf("  not run: a concept (see the README)\n");
    return def_iommu_holds() ? 0 : 1;
}

static int cleanup(void) {
    return 0;
}

static const attack_t attack = {
    "dma_attack",
    "a bus-master device reads or writes any RAM by DMA",
    INV_ISOLATION, SEV_HIGH,
    simulate, launch, cleanup,
    "iommu"
};

void atk_dma_attack_register(void) { registry_add_attack(&attack); }
