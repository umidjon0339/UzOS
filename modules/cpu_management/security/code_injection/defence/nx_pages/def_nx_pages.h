#ifndef DEF_NX_PAGES_H
#define DEF_NX_PAGES_H

// Code injection is beaten by making data pages non-executable (NX /
// W^X): the CPU refuses to run bytes the program wrote as data. 1 if
// UzOS's page model even has the bits to build on. NOTE: 32-bit paging
// without PAE has no execute bit, so UzOS cannot mark a page NX; this
// reports the honest limit.
int def_data_pages_could_be_nx(void);

#endif
