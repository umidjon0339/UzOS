#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>
#include "isr.h"

// Page Table / Directory Flags
#define PAGE_PRESENT  0x1
#define PAGE_WRITABLE 0x2
#define PAGE_USER     0x4

typedef uint32_t pde_t;
typedef uint32_t pte_t;

// Page Directory contains 1024 entries (4 KiB)
// Each Page Table contains 1024 entries (4 KiB)
#define PAGES_PER_TABLE     1024
#define TABLES_PER_DIR      1024
#define PAGE_SIZE           4096

typedef struct page_table {
    pte_t pages[PAGES_PER_TABLE];
} page_table_t;

typedef struct page_directory {
    pde_t tables[TABLES_PER_DIR];
} page_directory_t;

void vmm_init(void);

void vmm_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);
void vmm_unmap_page(uint32_t virt_addr);

void page_fault_handler(registers_t *regs);

#endif
