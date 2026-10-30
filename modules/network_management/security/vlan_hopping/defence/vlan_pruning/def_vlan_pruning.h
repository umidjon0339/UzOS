#ifndef DEF_VLAN_PRUNING_H
#define DEF_VLAN_PRUNING_H

/*
 * def_vlan_pruning.h - DEFENCE vlan_pruning (CONCEPT): trunks carry only the VLANs they need, never the native one untagged
 */

#include <stdint.h>

// Concept: 1 = the protection described in the README is in place.
int def_vlan_pruning_holds(void);

#endif
