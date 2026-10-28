#ifndef DEF_PAGE_SEPARATION_H
#define DEF_PAGE_SEPARATION_H

#include <stdint.h>

// 1 if a page-table entry 'pte' belongs to the kernel: it is present
// but has no PAGE_USER bit, so ring 3 may never read it. This is the
// line Meltdown tries to cross. The CPU enforces it on every access;
// UzOS sets it when it maps kernel memory (core/vmm, core/umem).
int def_kernel_page_hidden(uint32_t pte);

#endif
