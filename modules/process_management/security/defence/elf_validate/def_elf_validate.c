/*
 * def_elf_validate.c - DEFENCE elf_validate: never trust a file's numbers.
 *
 * Stops:  malicious_elf (security/attack/malicious_elf)
 * Called: elf_load_and_run() (core/elf), after reading the header and
 *         again after reading the program headers.
 * Rules:  header   - ELF magic, 32-bit, little-endian, executable, i386,
 *                    1..ELF_MAX_PHDRS program headers of the right size,
 *                    entry point in user space
 *         segments - file size <= memory size, and the whole range
 *                    [vaddr, vaddr + memsz) inside user space, no wrap
 * Why:    every number in an ELF file comes from whoever wrote the file.
 *         Without checks, a segment could load over the kernel at 1 MiB,
 *         wrap around the end of memory, or overflow the loader's array.
 */

#include "def_elf_validate.h"
#include "registry.h"
#include "audit.h"
#include "memory_layout.h"

// The first 4 bytes of every ELF file: 0x7F 'E' 'L' 'F'.
static int has_elf_magic(const uint8_t *ident) {
    return ident[0] == 0x7F && ident[1] == 'E' && ident[2] == 'L' && ident[3] == 'F';
}

static int in_user_space(uint32_t addr) {
    return addr >= USER_SPACE_START && addr < USER_SPACE_END;
}

// The header rule, as a pure function so the selftest can try it.
static int header_ok(const Elf32_Ehdr *h) {
    if (!has_elf_magic(h->e_ident)) return 0;
    if (h->e_ident[4] != ELF_CLASS_32) return 0;
    if (h->e_ident[5] != ELF_DATA_LSB) return 0;
    if (h->e_type != ELF_TYPE_EXEC) return 0;
    if (h->e_machine != ELF_MACHINE_386) return 0;
    if (h->e_phentsize != sizeof(Elf32_Phdr)) return 0;
    if (h->e_phnum == 0 || h->e_phnum > ELF_MAX_PHDRS) return 0;   // loader's array size
    return in_user_space(h->e_entry);
}

// The segment rule for one PT_LOAD.
static int segment_ok(const Elf32_Phdr *ph) {
    if (ph->p_type != PT_LOAD) return 1;                // not loaded: harmless
    if (ph->p_filesz > ph->p_memsz) return 0;           // would copy past the end
    if (!in_user_space(ph->p_vaddr)) return 0;          // starts outside
    return ph->p_memsz <= USER_SPACE_END - ph->p_vaddr; // ends outside or wraps
}

int def_elf_check_header(const Elf32_Ehdr *h) {
    if (header_ok(h))
        return 1;
    audit_log("elf_validate", "ELF header rejected");
    return 0;
}

int def_elf_check_segments(const Elf32_Phdr *ph, int count) {
    for (int i = 0; i < count; i++) {
        if (!segment_ok(&ph[i])) {
            audit_log("elf_validate", "ELF segment outside user space");
            return 0;
        }
    }
    return 1;
}

static int selftest(void) {
    // A good header and segment, like userland/hello.elf.
    Elf32_Ehdr h = {0};
    h.e_ident[0]   = 0x7F;
    h.e_ident[1]   = 'E';
    h.e_ident[2]   = 'L';
    h.e_ident[3]   = 'F';
    h.e_ident[4]   = ELF_CLASS_32;
    h.e_ident[5]   = ELF_DATA_LSB;
    h.e_type       = ELF_TYPE_EXEC;
    h.e_machine    = ELF_MACHINE_386;
    h.e_phentsize  = sizeof(Elf32_Phdr);
    h.e_phnum      = 3;
    h.e_entry      = 0x08049000;

    Elf32_Phdr ph = {0};
    ph.p_type   = PT_LOAD;
    ph.p_vaddr  = 0x08048000;
    ph.p_filesz = 0x100;
    ph.p_memsz  = 0x200;

    if (!header_ok(&h) || !segment_ok(&ph)) return 0;   // good file passes

    Elf32_Ehdr bad = h;
    bad.e_machine = 62;            if (header_ok(&bad)) return 0;  // x86-64
    bad = h; bad.e_phnum = 200;    if (header_ok(&bad)) return 0;  // array overflow
    bad = h; bad.e_entry = 0x100000; if (header_ok(&bad)) return 0; // jump into kernel

    Elf32_Phdr evil = ph;
    evil.p_vaddr = 0x00100000;     if (segment_ok(&evil)) return 0; // over the kernel
    evil = ph; evil.p_vaddr = 0xBFFFF000; evil.p_memsz = 0x2000;
                                   if (segment_ok(&evil)) return 0; // past user space
    evil = ph; evil.p_filesz = 0x300; if (segment_ok(&evil)) return 0; // file > memory
    return 1;
}

static const defence_t defence = {
    "elf_validate",
    "check every ELF header field before the loader uses it",
    "malicious_elf",
    selftest
};

void def_elf_validate_register(void) {
    registry_add_defence(&defence);
}
