#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>

#define KHEAP_MAGIC 0x14159265

#define KHEAP_START 0x01000000 // 16 MB virtual address mark
#define KHEAP_INITIAL_SIZE 0x00800000 // 8 MB initial heap size

typedef struct kheap_header {
    uint32_t magic;
    uint8_t  is_free;
    uint32_t size;
    struct kheap_header *next;
} kheap_header_t;

void kheap_init(void);

void *kmalloc(size_t size);
void *kmalloc_a(size_t size, int align);
void kfree(void *p);

#endif
