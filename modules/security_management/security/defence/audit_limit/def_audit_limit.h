#ifndef DEF_AUDIT_LIMIT_H
#define DEF_AUDIT_LIMIT_H

#include <stdint.h>

#define AUDIT_BURST         10      // events per defence that go into the ring...
#define AUDIT_WINDOW_TICKS  100     // ...per second (100 ticks); the rest are counted

// May one more event of a defence go into the audit ring? 'window_start'
// is when its current second began, 'stored' how many it put in since.
// 1 = keep it. 0 = only count it (core/audit then notes the flood once).
int def_audit_limit_allows(uint32_t now, uint32_t window_start, uint32_t stored);

#endif
