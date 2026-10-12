#include "kheap.h"
#include "vmm.h"
#include "pmm.h"
#include "vga.h"

static kheap_header_t *heap_start = NULL;

void kheap_init(void) {
    uint32_t pages = KHEAP_INITIAL_SIZE / PAGE_SIZE;

    for (uint32_t i = 0; i < pages; i++) {
        uint32_t virt = KHEAP_START + (i * PAGE_SIZE);
        void *phys = pmm_alloc_block();
        if (!phys) {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            vga_print_str("[KHEAP ERROR] Out of physical RAM for Kernel Heap!\n");
            return;
        }
        vmm_map_page(virt, (uint32_t)phys, PAGE_WRITABLE);
    }

    heap_start = (kheap_header_t *)KHEAP_START;
    heap_start->magic   = KHEAP_MAGIC;
    heap_start->is_free = 1;
    heap_start->size    = KHEAP_INITIAL_SIZE - sizeof(kheap_header_t);
    heap_start->next    = NULL;
}

void *kmalloc(size_t size) {
    if (size == 0 || !heap_start) return NULL;

    if (size % 4 != 0) {
        size += 4 - (size % 4);
    }

    kheap_header_t *curr = heap_start;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            if (curr->size >= size + sizeof(kheap_header_t) + 16) {
                kheap_header_t *new_block = (kheap_header_t *)((uint32_t)curr + sizeof(kheap_header_t) + size);
                new_block->magic   = KHEAP_MAGIC;
                new_block->is_free = 1;
                new_block->size    = curr->size - size - sizeof(kheap_header_t);
                new_block->next    = curr->next;

                curr->size = size;
                curr->next = new_block;
            }

            curr->is_free = 0;
            return (void *)((uint32_t)curr + sizeof(kheap_header_t));
        }
        curr = curr->next;
    }

    return NULL;
}

void *kmalloc_a(size_t size, int align) {
    if (!align) return kmalloc(size);

    void *ptr = kmalloc(size + PAGE_SIZE);
    if (!ptr) return NULL;

    uint32_t addr = (uint32_t)ptr;
    uint32_t aligned_addr = (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    return (void *)aligned_addr;
}

void kfree(void *p) {
    if (!p) return;

    kheap_header_t *header = (kheap_header_t *)((uint32_t)p - sizeof(kheap_header_t));
    if (header->magic != KHEAP_MAGIC) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("[KHEAP ERROR] Invalid kfree magic pointer!\n");
        return;
    }

    header->is_free = 1;

    kheap_header_t *curr = heap_start;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += sizeof(kheap_header_t) + curr->next->size;
            curr->next  = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}
