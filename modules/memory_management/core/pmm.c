#include "pmm.h"
#include "mem_security.h"
#include "vga.h"

extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

#define PMM_BITMAP_SIZE (PMM_MAX_BLOCKS / PMM_BLOCKS_PER_BYTE)

static uint8_t  pmm_bitmap[PMM_BITMAP_SIZE];
static uint32_t pmm_total_blocks = 0;
static uint32_t pmm_used_blocks  = 0;

static inline void mmap_set(uint32_t bit) {
    pmm_bitmap[bit / 8] |= (1 << (bit % 8));
}

static inline void mmap_unset(uint32_t bit) {
    pmm_bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline int mmap_test(uint32_t bit) {
    return pmm_bitmap[bit / 8] & (1 << (bit % 8));
}

static int32_t mmap_first_free(void) {
    for (uint32_t i = 0; i < pmm_total_blocks; i++) {
        if (!mmap_test(i)) {
            return (int32_t)i;
        }
    }
    return -1;
}

static int32_t mmap_first_free_s(size_t count) {
    if (count == 0) return -1;
    if (count == 1) return mmap_first_free();

    for (uint32_t i = 0; i < pmm_total_blocks; i++) {
        if (!mmap_test(i)) {
            uint32_t count_free = 0;
            for (uint32_t j = i; j < pmm_total_blocks; j++) {
                if (!mmap_test(j)) {
                    count_free++;
                } else {
                    break;
                }
                if (count_free == count) {
                    return (int32_t)i;
                }
            }
        }
    }
    return -1;
}


void pmm_init(multiboot_info_t *mb_info) {
    // 1. Mark all memory as USED by default (defensive)
    for (uint32_t i = 0; i < PMM_BITMAP_SIZE; i++) {
        pmm_bitmap[i] = 0xFF;
    }

    if (!(mb_info->flags & MULTIBOOT_INFO_MEM_MAP)) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("[PMM ERROR] Multiboot memory map info missing!\n");
        return;
    }

    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mb_info->mmap_addr;
    uint32_t mmap_end = mb_info->mmap_addr + mb_info->mmap_length;

    uint64_t highest_address = 0;

    // 2. Parse BIOS memory map: free AVAILABLE regions
    //    (use raw bitmap ops here to avoid broken counter math)
    while ((uint32_t)mmap < mmap_end) {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            uint64_t base = mmap->addr;
            uint64_t length = mmap->len;
            uint64_t align_base = base / PMM_BLOCK_SIZE;
            uint64_t blocks = length / PMM_BLOCK_SIZE;
            if (length % PMM_BLOCK_SIZE) blocks++;

            if (align_base < PMM_MAX_BLOCKS) {
                uint64_t available = PMM_MAX_BLOCKS - align_base;
                if (blocks > available) blocks = available;
                for (uint32_t b = 0; b < (uint32_t)blocks; b++)
                    mmap_unset((uint32_t)align_base + b);
            }

            if (mmap->addr + mmap->len > highest_address) {
                highest_address = mmap->addr + mmap->len;
            }
        }
        mmap = (multiboot_memory_map_t *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    uint64_t total_blocks = highest_address / PMM_BLOCK_SIZE;
    pmm_total_blocks = total_blocks > PMM_MAX_BLOCKS
        ? PMM_MAX_BLOCKS
        : (uint32_t)total_blocks;

    // 3. Mark lower 1 MB as USED (BIOS IVT, VGA at 0xA0000, etc.)
    {
        uint32_t blocks = 0x100000 / PMM_BLOCK_SIZE; // 256 blocks
        for (uint32_t b = 0; b < blocks; b++) {
            mmap_set(b);
        }
    }

    // 4. Mark Kernel code/data/bss sections as USED
    {
        uint32_t kernel_start = (uint32_t)&_kernel_start;
        uint32_t kernel_end   = (uint32_t)&_kernel_end;
        uint32_t align_base = kernel_start / PMM_BLOCK_SIZE;
        uint32_t blocks = (kernel_end - kernel_start + PMM_BLOCK_SIZE - 1) / PMM_BLOCK_SIZE;
        for (uint32_t b = 0; b < blocks; b++) {
            mmap_set(align_base + b);
        }
    }

    // 5. Count actual used blocks within valid RAM range
    pmm_used_blocks = 0;
    for (uint32_t i = 0; i < pmm_total_blocks; i++) {
        if (mmap_test(i)) {
            pmm_used_blocks++;
        }
    }
}


void *pmm_alloc_block(void) {
    if (pmm_get_free_blocks() <= 0) {
        return NULL;
    }

    int32_t frame = mmap_first_free();
    if (frame == -1) {
        return NULL;
    }

    mmap_set((uint32_t)frame);
    pmm_used_blocks++;

    return (void *)(frame * PMM_BLOCK_SIZE);
}

void pmm_free_block(void *p) {
    if (!mem_pmm_range_valid(p, 1)) return;

    uint32_t frame = (uint32_t)p / PMM_BLOCK_SIZE;

    if (mmap_test(frame)) {
        mmap_unset(frame);
        pmm_used_blocks--;
    }
}

void *pmm_alloc_blocks(size_t count) {
    if (count == 0) return NULL;
    if (pmm_get_free_blocks() < count) return NULL;

    int32_t frame = mmap_first_free_s(count);
    if (frame == -1) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        mmap_set((uint32_t)(frame + i));
        pmm_used_blocks++;
    }

    return (void *)(frame * PMM_BLOCK_SIZE);
}

void pmm_free_blocks(void *p, size_t count) {
    if (!mem_pmm_range_valid(p, count)) return;

    uint32_t frame = (uint32_t)p / PMM_BLOCK_SIZE;

    for (size_t i = 0; i < count; i++) {
        if (mmap_test((uint32_t)(frame + i))) {
            mmap_unset((uint32_t)(frame + i));
            pmm_used_blocks--;
        }
    }
}

uint32_t pmm_get_max_blocks(void) {
    return pmm_total_blocks;
}

uint32_t pmm_get_used_blocks(void) {
    return pmm_used_blocks;
}

uint32_t pmm_get_free_blocks(void) {
    return pmm_total_blocks - pmm_used_blocks;
}
