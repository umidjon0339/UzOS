#ifndef DEF_GUARD_GAP_H
#define DEF_GUARD_GAP_H

#include <stdint.h>
#include "modules/process_management/core/elf/elf.h"   // Elf32_Phdr

#define GUARD_PAGES 8           // unmapped pages kept below every execve stack (32 KiB)

// execve: does any PT_LOAD segment of a new program reach into the
// execve stack area (EXEC_STACKS_START..END)? 1 = clear, 0 = refused (logged).
int def_segments_clear_of_stacks(const Elf32_Phdr *ph, int count);

// execve: are the GUARD_PAGES below a new stack (lowest address
// 'stack_bottom') unmapped, so a growing stack faults instead of running
// into other memory? 1 = yes, 0 = refused (logged).
int def_guard_gap_ok(uint32_t stack_bottom);

#endif
