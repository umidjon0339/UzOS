#include "elf.h"
#include "vfs.h"
#include "pmm.h"
#include "vmm.h"
#include "vga.h"
#include "string.h"
#include "process.h"

int elf_load_and_run(const char *filename) {
    vfs_node_t *node = vfs_finddir(fs_root, filename);
    if (!node) {
        vga_print_str("ELF Loader: File not found.\n");
        return -1;
    }

    Elf32_Ehdr ehdr;
    if (vfs_read(node, 0, sizeof(Elf32_Ehdr), (uint8_t*)&ehdr) != sizeof(Elf32_Ehdr)) {
        vga_print_str("ELF Loader: Failed to read ELF header.\n");
        return -1;
    }

    // Verify Magic Number
    uint32_t *magic = (uint32_t*)ehdr.e_ident;
    if (*magic != ELF_MAGIC) {
        vga_print_str("ELF Loader: Invalid ELF magic number.\n");
        return -1;
    }

    // Read Program Headers
    Elf32_Phdr phdrs[16]; // Assume max 16 segments for simple ELFs
    if (ehdr.e_phnum > 16) {
        vga_print_str("ELF Loader: Too many program headers.\n");
        return -1;
    }

    if (vfs_read(node, ehdr.e_phoff, ehdr.e_phnum * sizeof(Elf32_Phdr), (uint8_t*)phdrs) != (ehdr.e_phnum * sizeof(Elf32_Phdr))) {
        vga_print_str("ELF Loader: Failed to read program headers.\n");
        return -1;
    }

    // Load Segments
    for (int i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            uint32_t memsz = phdrs[i].p_memsz;
            uint32_t filesz = phdrs[i].p_filesz;
            uint32_t vaddr = phdrs[i].p_vaddr;

            // Page align addresses
            uint32_t page_start = vaddr & ~(PAGE_SIZE - 1);
            uint32_t page_end = (vaddr + memsz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
            uint32_t num_pages = (page_end - page_start) / PAGE_SIZE;

            // Map physical pages
            for (uint32_t p = 0; p < num_pages; p++) {
                uint32_t paddr = (uint32_t)pmm_alloc_block();
                vmm_map_page(page_start + p * PAGE_SIZE, paddr, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
            }

            // Zero out BSS
            memset((void*)vaddr, 0, memsz);

            // Copy file data to memory
            if (filesz > 0) {
                vfs_read(node, phdrs[i].p_offset, filesz, (uint8_t*)vaddr);
            }
        }
    }

    vga_print_str("ELF loaded successfully. Starting process...\n");

    // Start process (running in Ring 0 for now)
    int pid = process_create(filename, (void (*)(void))ehdr.e_entry);
    return pid;
}
