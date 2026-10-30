#ifndef DEF_RESPONSE_LIMIT_H
#define DEF_RESPONSE_LIMIT_H

/*
 * def_response_limit.h - DEFENCE response_limit (CONCEPT): a server limits answers per client and never answers much bigger than asked
 */

#include <stdint.h>

// Concept: 1 = the protection described in the README is in place.
int def_response_limit_holds(void);

#endif
