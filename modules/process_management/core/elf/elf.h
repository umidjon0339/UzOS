#ifndef ELF_H
#define ELF_H

/*
 * elf.h - ELF, the file format of Linux/UzOS programs.
 *
 * An ELF file starts with a header (Elf32_Ehdr) that says what CPU it is
 * for and where the program headers are. Each program header
 * (Elf32_Phdr) of type PT_LOAD says: "copy these bytes of the file to
 * this address in memory". Then the CPU jumps to e_entry.
 */

#include <stdint.h>

#define ELF_CLASS_32   1            // e_ident[4]: 32-bit
#define ELF_DATA_LSB   1            // e_ident[5]: little-endian
#define ELF_TYPE_EXEC  2            // e_type: executable
#define ELF_MACHINE_386 3           // e_machine: Intel 80386
#define PT_LOAD        1            // p_type: load this segment

#define ELF_MAX_PHDRS  16           // more program headers than this: refuse

typedef struct {
    uint8_t  e_ident[16];   // 0x7F 'E' 'L' 'F', then class, byte order, ...
    uint16_t e_type;        // executable, shared library, ...
    uint16_t e_machine;     // which CPU
    uint32_t e_version;
    uint32_t e_entry;       // address of the first instruction
    uint32_t e_phoff;       // file offset of the program headers
    uint32_t e_shoff;       // section headers (not needed to run)
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;   // size of one program header
    uint16_t e_phnum;       // number of program headers
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) Elf32_Ehdr;

typedef struct {
    uint32_t p_type;        // PT_LOAD = copy into memory
    uint32_t p_offset;      // where the bytes are in the file
    uint32_t p_vaddr;       // where they go in memory
    uint32_t p_paddr;
    uint32_t p_filesz;      // bytes taken from the file
    uint32_t p_memsz;       // bytes in memory (the rest is zeroed: .bss)
    uint32_t p_flags;
    uint32_t p_align;
} __attribute__((packed)) Elf32_Phdr;

// Load an ELF file (relative or absolute path) and start it as a process.
// Returns the new PID, or -1 on error.
int elf_load_and_run(const char *filename);

#endif
