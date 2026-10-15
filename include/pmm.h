#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include "multiboot.h"

#define PMM_BLOCK_SIZE          4096 // 4 KiB
#define PMM_BLOCKS_PER_BYTE     8
#define PMM_MAX_BLOCKS           1048576

void pmm_init(multiboot_info_t *mb_info);

void *pmm_alloc_block(void);
void pmm_free_block(void *p);

void *pmm_alloc_blocks(size_t count);
void pmm_free_blocks(void *p, size_t count);

uint32_t pmm_get_max_blocks(void);
uint32_t pmm_get_used_blocks(void);
uint32_t pmm_get_free_blocks(void);

#endif
