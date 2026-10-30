#ifndef DEF_IOMMU_H
#define DEF_IOMMU_H

/*
 * def_iommu.h - DEFENCE iommu (CONCEPT): an IOMMU limits each device to the memory mapped for it
 */

#include <stdint.h>

// Concept: 1 = the protection described in the README is in place.
int def_iommu_holds(void);

#endif
