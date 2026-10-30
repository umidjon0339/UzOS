#ifndef DEF_KPTR_HIDE_H
#define DEF_KPTR_HIDE_H

#include <stdint.h>

// The kernel address 'addr' as user 'viewer_uid' may see it: itself for
// CAP_PTRACE (root), 0 for everyone else. Every place that prints a
// kernel address (procinfo: kernel stack, saved ESP, entry function)
// passes it through this first.
uint32_t def_kptr_show(uint32_t viewer_uid, uint32_t addr);

#endif
