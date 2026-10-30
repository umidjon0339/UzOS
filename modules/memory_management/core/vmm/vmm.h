#ifndef VMM_H
#define VMM_H

/*
 * vmm.h - Virtual Memory Manager: paging.
 *
 * x86 paging translates a 32-bit virtual address in two steps:
 *
 *   virtual address:  | 10 bits        | 10 bits      | 12 bits |
 *                     | directory index| table index  | offset  |
 *
 *   CR3 -> page directory (1024 entries)
 *            entry -> page table (1024 entries)
 *                       entry -> 4 KiB physical frame + offset
 *
 * Each entry holds a frame address plus flag bits (present, writable,
 * user). A missing entry, or a forbidden access, raises a page fault.
 */

#include <stdint.h>
#include "memory_layout.h"

// Flag bits in directory and table entries
#define PAGE_PRESENT  0x1   // the entry is valid
#define PAGE_WRITABLE 0x2   // writes allowed (else read-only)
#define PAGE_USER     0x4   // ring 3 may access (else kernel only)
#define PAGE_NO_CACHE 0x10  // PCD: never cache (device registers, e.g. the network card)
#define PAGE_FLAGS    0xFFF // low 12 bits of an entry are flags

#define PAGES_PER_TABLE 1024
#define TABLES_PER_DIR  1024

typedef uint32_t pde_t;     // page directory entry
typedef uint32_t pte_t;     // page table entry

typedef struct { pte_t pages[PAGES_PER_TABLE]; } page_table_t;
typedef struct { pde_t tables[TABLES_PER_DIR]; } page_directory_t;

// The result of following one address through the page tables.
typedef struct {
    uint32_t dir_index;     // which page directory entry
    uint32_t table_index;   // which page table entry
    pde_t    pde;           // the directory entry (0 = no table)
    pte_t    pte;           // the table entry (0 = not mapped)
    uint32_t physical;      // the physical address, if mapped
} vmm_walk_t;

// Build the kernel's page tables and turn paging on.
void vmm_init(void);

// Map one page. Returns 1 on success, 0 if no frame was free for a new table.
int vmm_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);

// Remove one page's mapping (the frame itself is not freed).
void vmm_unmap_page(uint32_t virt_addr);

// Follow 'virt_addr' through the tables. Returns 1 if it is mapped.
int vmm_walk(uint32_t virt_addr, vmm_walk_t *out);

#endif
