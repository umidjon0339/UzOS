#ifndef DEF_NULL_PAGE_H
#define DEF_NULL_PAGE_H

// Unmap page 0, so any NULL pointer access raises a page fault.
// Called once by vmm_init().
void def_null_page_protect(void);

// Is page 0 still unmapped? 1 = yes.
int def_null_page_ok(void);

#endif
