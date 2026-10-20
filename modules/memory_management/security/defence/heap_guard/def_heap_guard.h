#ifndef DEF_HEAP_GUARD_H
#define DEF_HEAP_GUARD_H

// May kfree() free 'p'? 1 = yes, 0 = refused (logged).
int def_kfree_ok(const void *p);

#endif
