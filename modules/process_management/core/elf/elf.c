/*
 * elf.c - Load an ELF program from the filesystem and start it.
 *
 *   1. read the ELF header          -> check it (elf_validate)
 *   2. read the program headers     -> check them (elf_validate)
 *   3. for each PT_LOAD segment: map pages, zero them, copy the bytes
 *   4. start a process at e_entry
 *
 * Running a program needs read permission on its file (perm_check), and
 * a kernel whose code is still the code it booted with (integrity_check).
 *
 * Uses: vfs_lookup/vfs_read (filesystem_management), perm_check
 *       (user_management), integrity_verify (security_management),
 *       pmm_alloc_block +
 *       vmm_map_page (memory_management), process_create (core/process),
 *       ui_error (ui module) to say why a program was refused.
 *
 * The program belongs to the user who started it. Limits for now: all
 * processes share one address space, and the program runs in ring 0.
 * Running the same program twice maps fresh pages over the first copy.
 */

#include "elf.h"
#include "vfs.h"
#include "perm.h"
#include "pmm.h"
#include "vmm.h"
#include "ui.h"
#include "string.h"
#include "process.h"
#include "def_elf_validate.h"
#include "integrity.h"


static int fail(const char *reason) {
    ui_error("elf", reason);
    return -1;
}

// Map the pages a segment covers, zero them, copy its bytes from the file.
static int load_segment(vfs_node_t *file, const Elf32_Phdr *ph) {
    uint32_t first_page = ph->p_vaddr & ~(PAGE_SIZE - 1);
    uint32_t end_page   = (ph->p_vaddr + ph->p_memsz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    for (uint32_t page = first_page; page < end_page; page += PAGE_SIZE) {
        void *frame = pmm_alloc_block();
        if (!frame || !vmm_map_page(page, (uint32_t)frame, PAGE_WRITABLE | PAGE_USER))
            return 0;   // out of physical memory
    }

    // Zero everything first: the part past p_filesz (.bss) must start at 0.
    memset((void *)ph->p_vaddr, 0, ph->p_memsz);

    if (ph->p_filesz == 0)
        return 1;
    return vfs_read(file, ph->p_offset, ph->p_filesz, (uint8_t *)ph->p_vaddr) == ph->p_filesz;
}

int elf_load_and_run(const char *filename) {
    // DEFENCE integrity_check: a patched kernel starts no programs.
    if (!integrity_verify())
        return fail("refused: the kernel's code changed since boot (see `integrity`)");

    vfs_node_t *file = vfs_lookup(vfs_cwd(), filename);
    if (!file || !(file->flags & FS_FILE))
        return fail("file not found");
    if (!perm_check(file, PERM_READ))
        return fail("permission denied");

    Elf32_Ehdr ehdr;
    if (vfs_read(file, 0, sizeof(ehdr), (uint8_t *)&ehdr) != sizeof(ehdr))
        return fail("cannot read the ELF header");

    // DEFENCE elf_validate: no number from the file is used before this.
    if (!def_elf_check_header(&ehdr))
        return fail("rejected: not a valid i386 executable");

    Elf32_Phdr phdrs[ELF_MAX_PHDRS];
    uint32_t phdrs_size = ehdr.e_phnum * sizeof(Elf32_Phdr);   // <= 16, checked above
    if (vfs_read(file, ehdr.e_phoff, phdrs_size, (uint8_t *)phdrs) != phdrs_size)
        return fail("cannot read the program headers");

    // DEFENCE elf_validate: every segment must stay inside user space.
    if (!def_elf_check_segments(phdrs, ehdr.e_phnum))
        return fail("rejected: a segment points outside user space");

    for (int i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD && !load_segment(file, &phdrs[i]))
            return fail("out of memory or short read while loading");
    }

    return process_create(filename, (void (*)(void))ehdr.e_entry, process_caller_uid());
}
