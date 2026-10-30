/*
 * atk_malicious_elf.c - ATTACK malicious_elf: a crafted program file.
 *
 * Breaks:     trust - numbers read from a file must be checked first
 * Stopped by: elf_validate (defence/elf_validate)
 *
 * The ELF loader copies bytes to whatever address the file names. A
 * crafted file can name the kernel's own code, wrap around the end of
 * memory, or claim more program headers than the loader can hold.
 *
 * simulate(): list the tricks.
 * launch():   build each crafted header in memory and pass it through the
 *             same checks elf_load_and_run() uses. Nothing is loaded.
 *
 * Uses: def_elf_check_header / def_elf_check_segments
 *       (defence/elf_validate), Elf32_* types (core/elf).
 */

#include "modules/security_management/core/registry/registry.h"                                       // attack_t, registry_add_attack
#include "modules/device_management/vga/vga.h"                                                        // vga_print_str, vga_print_int
#include "modules/process_management/core/elf/elf.h"                                                  // ELF_CLASS_32, ELF_DATA_LSB, ELF_TYPE_EXEC
#include "modules/process_management/security/malicious_elf/defence/elf_validate/def_elf_validate.h"  // def_elf_check_segments, def_elf_check_header

// A valid header and segment (like userland/hello.elf) to start from.
static Elf32_Ehdr good_header(void) {
    Elf32_Ehdr h = {0};
    h.e_ident[0]  = 0x7F;
    h.e_ident[1]  = 'E';
    h.e_ident[2]  = 'L';
    h.e_ident[3]  = 'F';
    h.e_ident[4]  = ELF_CLASS_32;
    h.e_ident[5]  = ELF_DATA_LSB;
    h.e_type      = ELF_TYPE_EXEC;
    h.e_machine   = ELF_MACHINE_386;
    h.e_phentsize = sizeof(Elf32_Phdr);
    h.e_phnum     = 1;
    h.e_entry     = 0x08049000;
    return h;
}

static Elf32_Phdr good_segment(void) {
    Elf32_Phdr ph = {0};
    ph.p_type   = PT_LOAD;
    ph.p_vaddr  = 0x08048000;
    ph.p_filesz = 0x1000;
    ph.p_memsz  = 0x1000;
    return ph;
}

static void report(const char *trick, int accepted) {
    vga_print_str("  ");
    vga_print_str(trick);
    vga_print_str(accepted ? "  -> ACCEPTED\n" : "  -> refused\n");
}

// Run one crafted segment through the loader's check. Returns 1 if refused.
static int try_segment(const char *trick, Elf32_Phdr ph) {
    int accepted = def_elf_check_segments(&ph, 1);
    report(trick, accepted);
    return !accepted;
}

// Run one crafted header through the loader's check. Returns 1 if refused.
static int try_header(const char *trick, Elf32_Ehdr h) {
    int accepted = def_elf_check_header(&h);
    report(trick, accepted);
    return !accepted;
}

static int simulate(void) {
    vga_print_str("  segment at 0x00100000      overwrite the kernel's code\n");
    vga_print_str("  segment 0xFFFFF000+0x2000  wrap past 4 GiB to low memory\n");
    vga_print_str("  filesz > memsz             copy past the segment's end\n");
    vga_print_str("  e_phnum = 200              overflow the loader's 16 slots\n");
    vga_print_str("  entry = 0x00100000         jump straight into the kernel\n");
    return 5;
}

static int launch(void) {
    int refused = 0;
    Elf32_Phdr ph;
    Elf32_Ehdr h;

    ph = good_segment(); ph.p_vaddr = 0x00100000;
    refused += try_segment("segment over the kernel", ph);

    ph = good_segment(); ph.p_vaddr = 0xFFFFF000; ph.p_memsz = 0x2000;
    refused += try_segment("wrapping segment       ", ph);

    ph = good_segment(); ph.p_filesz = 0x2000;
    refused += try_segment("filesz > memsz         ", ph);

    h = good_header(); h.e_phnum = 200;
    refused += try_header("200 program headers    ", h);

    h = good_header(); h.e_entry = 0x00100000;
    refused += try_header("entry in the kernel    ", h);

    // A normal program must still pass both checks.
    try_header("normal program header  ", good_header());
    try_segment("normal program segment ", good_segment());

    vga_print_str("  crafted files refused: ");
    vga_print_int(refused);
    vga_print_str(" of 5\n");
    return refused;
}

static const attack_t attack = {
    "malicious_elf",
    "a crafted ELF file loads over the kernel or overflows the loader",
    INV_TRUST, SEV_CRITICAL,
    simulate, launch, 0,        // nothing loaded, nothing to undo
    "elf_validate"
};

void atk_malicious_elf_register(void) {
    registry_add_attack(&attack);
}
