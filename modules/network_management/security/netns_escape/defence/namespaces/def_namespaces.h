#ifndef DEF_NAMESPACES_H
#define DEF_NAMESPACES_H

/*
 * def_namespaces.h - DEFENCE namespaces (CONCEPT): each container gets its own network namespace, and never the host's capabilities
 */

#include <stdint.h>

// Concept: 1 = the protection described in the README is in place.
int def_namespaces_holds(void);

#endif
