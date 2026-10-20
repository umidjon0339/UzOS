/*
 * kheap.c - First-fit kernel heap.
 *
 * Interrupts are turned off inside kmalloc() and kfree(): the reaper
 * frees stacks from the timer interrupt, and an interrupt landing in the
 * middle of a list update would see a half-changed list.
 *
 * Uses: pmm_alloc_block + vmm_map_page (core/pmm, core/vmm) to back the
 *       heap with RAM, cpu_irq_save (arch/x86/cpu), and two defences:
 *         heap_canary - every header is checked before it is trusted
 *         heap_guard  - kfree() only accepts real, in-use blocks
 */

#include "kheap.h"
#include "pmm.h"
#include "vmm.h"
#include "cpu.h"
#include "vga.h"
#include "def_heap_canary.h"
#include "def_heap_guard.h"

#define ALIGN          8                    // every block starts 8-byte aligned
#define MIN_SPLIT      16                   // don't split off smaller leftovers
#define HEADER_SIZE    sizeof(kheap_header_t)

static kheap_header_t *heap_start;

static uint32_t round_up(uint32_t n) {
    return (n + ALIGN - 1) & ~(ALIGN - 1);
}

static void *data_of(kheap_header_t *h) {
    return (uint8_t *)h + HEADER_SIZE;
}

void kheap_init(void) {
    for (uint32_t offset = 0; offset < KHEAP_SIZE; offset += PAGE_SIZE) {
        void *frame = pmm_alloc_block();
        if (!frame || !vmm_map_page(KHEAP_START + offset, (uint32_t)frame, PAGE_WRITABLE)) {
            vga_print_str("[KHEAP ERROR] Out of RAM for the kernel heap!\n");
            return;
        }
    }

    heap_start = (kheap_header_t *)KHEAP_START;
    heap_start->magic   = KHEAP_MAGIC;
    heap_start->size    = KHEAP_SIZE - HEADER_SIZE;
    heap_start->is_free = 1;
    heap_start->next    = 0;
}

void *kmalloc(size_t size) {
    if (size == 0 || size > KHEAP_SIZE || !heap_start)
        return 0;
    size = round_up(size);

    uint32_t irq = cpu_irq_save();
    void *result = 0;

    for (kheap_header_t *b = heap_start; b; b = b->next) {
        // DEFENCE heap_canary: never follow a header that was overwritten.
        if (!def_heap_block_ok(b))
            break;
        if (!b->is_free || b->size < size)
            continue;

        // Big enough to split? Then the rest becomes a new free block.
        if (b->size >= size + HEADER_SIZE + MIN_SPLIT) {
            kheap_header_t *rest = (kheap_header_t *)((uint8_t *)data_of(b) + size);
            rest->magic   = KHEAP_MAGIC;
            rest->size    = b->size - size - HEADER_SIZE;
            rest->is_free = 1;
            rest->next    = b->next;

            b->size = size;
            b->next = rest;
        }

        b->is_free = 0;
        result = data_of(b);
        break;
    }

    cpu_irq_restore(irq);
    return result;
}

void kfree(void *p) {
    if (!p)
        return;                     // kfree(NULL) does nothing, like free()

    uint32_t irq = cpu_irq_save();

    // DEFENCE heap_guard: a real block start, inside the heap, in use.
    if (def_kfree_ok(p)) {
        ((kheap_header_t *)((uint8_t *)p - HEADER_SIZE))->is_free = 1;

        // Merge every pair of neighbouring free blocks into one.
        kheap_header_t *b = heap_start;
        while (b && b->next) {
            // DEFENCE heap_canary: stop before trusting a broken header.
            if (!def_heap_block_ok(b->next))
                break;
            if (b->is_free && b->next->is_free) {
                b->size += HEADER_SIZE + b->next->size;
                b->next  = b->next->next;
            } else {
                b = b->next;
            }
        }
    }

    cpu_irq_restore(irq);
}

void kheap_get_stats(kheap_stats_t *out) {
    uint32_t irq = cpu_irq_save();
    *out = (kheap_stats_t){0};

    for (kheap_header_t *b = heap_start; b; b = b->next) {
        if (b->magic != KHEAP_MAGIC) {      // just report it, no logging here
            out->corrupted = 1;
            break;
        }
        if (b->is_free) {
            out->blocks_free++;
            out->bytes_free += b->size;
            if (b->size > out->largest_free)
                out->largest_free = b->size;
        } else {
            out->blocks_used++;
            out->bytes_used += b->size;
        }
    }

    cpu_irq_restore(irq);
}
