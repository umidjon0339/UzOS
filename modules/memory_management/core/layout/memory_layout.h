#ifndef MEMORY_LAYOUT_H
#define MEMORY_LAYOUT_H

/*
 * memory_layout.h - Where everything lives in the 4 GiB address space.
 *
 * With paging on, every address the CPU uses is VIRTUAL. The VMM's page
 * tables translate it to a PHYSICAL address in RAM. UzOS uses one address
 * space for everything:
 *
 *   0x00000000  +---------------------------+
 *               | NULL page: NOT mapped     |  any NULL access faults
 *   0x00001000  +---------------------------+
 *               | identity mapped           |  virtual == physical:
 *               |  BIOS, VGA 0xB8000,       |  the kernel can reach these
 *               |  kernel at 1 MiB, ...     |  by their physical address
 *   0x00800000  +---------------------------+
 *               | (not mapped)              |
 *   0x01000000  +---------------------------+
 *               | kernel heap (kmalloc)     |  8 MiB
 *   0x01800000  +---------------------------+
 *               | (not mapped)              |
 *   0x08000000  +===========================+  USER SPACE starts
 *               |  0x08048000 ELF programs  |
 *               |  0x40000000 testuser      |
 *               |  0x50000000 sbrk heap     |  grows up
 *               |  0x60000000 mmap area     |
 *   0xC0000000  +===========================+  USER SPACE ends
 *               | (not mapped)              |
 *   0xFD000000  +---------------------------+
 *               | BGA framebuffer (GUI)     |  identity mapped, 8 MiB
 *   0xFFC00000  +---------------------------+
 *               | the page tables themselves|  "recursive" mapping
 *   0xFFFFFFFF  +---------------------------+
 */

#define PAGE_SIZE           0x1000      // 4 KiB: the unit paging works in

#define NULL_PAGE_END       0x00001000
#define IDENTITY_MAP_END    0x00800000  // first 8 MiB: virtual == physical

#define KHEAP_START         0x01000000
#define KHEAP_SIZE          0x00800000  // 8 MiB

#define USER_SPACE_START    0x08000000
#define USER_TEST_CODE      0x40000000  // `testuser`: code page, then stack page
#define USER_BRK_START      0x50000000  // sbrk heap
#define USER_BRK_END        0x60000000
#define MMAP_START          0x60000000  // mmap regions
#define MMAP_END            0x80000000
#define USER_SPACE_END      0xC0000000

#define FRAMEBUFFER_START   0xFD000000  // QEMU BGA video memory
#define FRAMEBUFFER_END     0xFD800000

#define PAGE_TABLES_VIRT    0xFFC00000  // page table i is at + i * PAGE_SIZE

#endif
