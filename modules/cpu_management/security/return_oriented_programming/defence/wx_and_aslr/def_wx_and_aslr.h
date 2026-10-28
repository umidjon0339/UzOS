#ifndef DEF_WX_AND_ASLR_H
#define DEF_WX_AND_ASLR_H

// The real defences against ROP are W^X (no page both writable and
// executable), ASLR (addresses unpredictable) and CFI (returns checked).
// 1 if UzOS has the page bits these build on (PAGE_WRITABLE, PAGE_USER).
// NOTE: 32-bit paging without PAE has no no-execute bit, so UzOS cannot
// enforce W^X; this reports what the page model does and does not offer.
int def_page_bits_present(void);

#endif
