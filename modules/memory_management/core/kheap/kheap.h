#ifndef KHEAP_H
#define KHEAP_H

/*
 * kheap.h - Kernel heap: kmalloc() and kfree().
 *
 * The heap is one 8 MiB area (KHEAP_START) cut into blocks. Every block
 * starts with a header, and the headers form a linked list in address
 * order:
 *
 *   [header|  data  ][header| data ][header|      data       ] ...
 *     used            free            used
 *
 * kmalloc() takes the first free block that is big enough (splitting off
 * the rest). kfree() marks a block free and merges free neighbours.
 */

#include <stdint.h>
#include <stddef.h>
#include "memory_layout.h"

#define KHEAP_MAGIC 0x14159265      // in every header: "this really is a block"

typedef struct kheap_header {
    uint32_t magic;                 // KHEAP_MAGIC, else the header was overwritten
    uint32_t size;                  // bytes of data after the header
    uint32_t is_free;               // 1 = free, 0 = in use
    struct kheap_header *next;      // next block in address order (NULL = last)
} kheap_header_t;

typedef struct {
    uint32_t blocks_used, blocks_free;
    uint32_t bytes_used,  bytes_free;
    uint32_t largest_free;          // biggest kmalloc() that can succeed now
    uint32_t corrupted;             // 1 if a broken header stopped the walk
} kheap_stats_t;

// Map the heap's pages and make it one big free block.
void kheap_init(void);

// size bytes of kernel memory, or NULL. Not zeroed.
void *kmalloc(size_t size);

// Give memory from kmalloc() back. Bad frees are refused (see heap_guard).
void kfree(void *p);

void kheap_get_stats(kheap_stats_t *out);

#endif
