#ifndef DEF_USER_KERNEL_ISOLATION_H
#define DEF_USER_KERNEL_ISOLATION_H

// 1 if the user/kernel boundary is in place: kernel memory is mapped
// without the user bit, so neither a normal nor a speculative ring 3
// access may read it. This is the boundary Spectre tries to make the
// CPU step over speculatively. Checked against the real page tables
// (core/vmm).
int def_isolation_boundary_ok(void);

#endif
