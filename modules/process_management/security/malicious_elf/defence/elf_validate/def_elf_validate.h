#ifndef DEF_ELF_VALIDATE_H
#define DEF_ELF_VALIDATE_H

#include "elf.h"

// Is this a well-formed i386 executable we can load? 1 = yes, 0 = refused.
int def_elf_check_header(const Elf32_Ehdr *h);

// Do all PT_LOAD segments stay inside user space? 1 = yes, 0 = refused.
int def_elf_check_segments(const Elf32_Phdr *ph, int count);

#endif
