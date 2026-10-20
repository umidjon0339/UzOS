/*
 * pmm.c - Track which 4 KiB frames of physical RAM are free.
 *
 *   bitmap bit = 1  frame is used (or not RAM at all)
 *   bitmap bit = 0  frame is free
 *
 * At boot every frame starts USED. Then the frames the bootloader calls
 * "available" are set free, and finally everything that must never be
 * handed out is reserved again: firmware areas, the first 1 MiB, the
 * kernel image, and the multiboot data the kernel was given.
 *
 * Uses: the multiboot memory map (boot/multiboot.h), _kernel_start/_end
 *       (boot/linker.ld), cpu_irq_save (arch/x86/cpu), and the defence
 *       pmm_free_guard for every free.
 */

#include "pmm.h"
#include "cpu.h"
#include "vga.h"
#include "def_pmm_free_guard.h"

#define LOW_MEMORY_END    0x100000      // first 1 MiB: BIOS, VGA, ...
#define MAX_RESERVED      32

extern uint8_t _kernel_start[];
extern uint8_t _kernel_end[];

static uint8_t  bitmap[PMM_MAX_FRAMES / 8];
static uint32_t total_frames;
static uint32_t used_frames;
static uint64_t highest_ram;       // end of the highest usable RAM region

// Frame ranges [first, end) that may never be freed.
static struct { uint32_t first, end; } reserved[MAX_RESERVED];
static int reserved_count;

static pmm_region_t regions[PMM_MAX_REGIONS];
static int region_count;

/* ---------- Bitmap ---------- */

static void mark_used(uint32_t frame) { bitmap[frame / 8] |=  (1 << (frame % 8)); }
static void mark_free(uint32_t frame) { bitmap[frame / 8] &= ~(1 << (frame % 8)); }
static int  is_used(uint32_t frame)   { return bitmap[frame / 8] & (1 << (frame % 8)); }

static uint32_t to_frame(uint64_t address) {
    uint64_t frame = address / PAGE_SIZE;
    return frame < PMM_MAX_FRAMES ? (uint32_t)frame : PMM_MAX_FRAMES;
}

/* ---------- Setup ---------- */

// Free RAM: round INWARD, so a frame that is only partly RAM stays used.
static void free_range(uint64_t start, uint64_t end) {
    uint32_t first = to_frame(start + PAGE_SIZE - 1);
    uint32_t last  = to_frame(end);
    for (uint32_t f = first; f < last; f++)
        mark_free(f);
}

// Reserved: round OUTWARD, so a frame that is even partly reserved is kept.
static void reserve_range(uint64_t start, uint64_t end) {
    if (start >= end)
        return;

    uint32_t first = to_frame(start);
    uint32_t last  = to_frame(end + PAGE_SIZE - 1);
    for (uint32_t f = first; f < last; f++)
        mark_used(f);

    if (reserved_count < MAX_RESERVED) {
        reserved[reserved_count].first = first;
        reserved[reserved_count].end   = last;
        reserved_count++;
    }
}

// Call 'visit' for every entry of the bootloader's memory map.
static void for_each_mmap_entry(multiboot_info_t *mb, void (*visit)(multiboot_memory_map_t *)) {
    uint32_t addr = mb->mmap_addr;
    uint32_t end  = mb->mmap_addr + mb->mmap_length;

    while (addr < end) {
        multiboot_memory_map_t *e = (multiboot_memory_map_t *)addr;
        visit(e);
        addr += e->size + sizeof(e->size);  // 'size' does not count itself
    }
}

static void add_available(multiboot_memory_map_t *e) {
    if (region_count < PMM_MAX_REGIONS) {
        regions[region_count].base   = e->addr;
        regions[region_count].length = e->len;
        regions[region_count].type   = e->type;
        region_count++;
    }

    if (e->type != MULTIBOOT_MEMORY_AVAILABLE)
        return;
    free_range(e->addr, e->addr + e->len);
    if (e->addr + e->len > highest_ram)
        highest_ram = e->addr + e->len;
}

static void add_reserved(multiboot_memory_map_t *e) {
    if (e->type != MULTIBOOT_MEMORY_AVAILABLE)
        reserve_range(e->addr, e->addr + e->len);
}

// The kernel image and everything the bootloader handed us.
static void reserve_boot_data(multiboot_info_t *mb) {
    reserve_range(0, LOW_MEMORY_END);
    reserve_range((uint32_t)_kernel_start, (uint32_t)_kernel_end);
    reserve_range((uint32_t)mb, (uint32_t)mb + sizeof(*mb));
    reserve_range(mb->mmap_addr, mb->mmap_addr + mb->mmap_length);

    if (mb->flags & MULTIBOOT_INFO_MODS) {
        multiboot_module_t *mods = (multiboot_module_t *)mb->mods_addr;
        reserve_range(mb->mods_addr, mb->mods_addr + mb->mods_count * sizeof(*mods));
        for (uint32_t i = 0; i < mb->mods_count; i++)
            reserve_range(mods[i].mod_start, mods[i].mod_end);
    }
}

void pmm_init(multiboot_info_t *mb) {
    // 1. Everything starts used: unknown memory is never handed out.
    for (uint32_t i = 0; i < sizeof(bitmap); i++)
        bitmap[i] = 0xFF;

    if (!(mb->flags & MULTIBOOT_INFO_MEM_MAP)) {
        vga_print_str("[PMM ERROR] The bootloader gave no memory map!\n");
        return;
    }

    // 2. Free what the bootloader calls available RAM.
    for_each_mmap_entry(mb, add_available);
    total_frames = to_frame(highest_ram);

    // 3. Reserve again what must never be used, even inside "available".
    for_each_mmap_entry(mb, add_reserved);
    reserve_boot_data(mb);

    // 4. Count.
    used_frames = 0;
    for (uint32_t f = 0; f < total_frames; f++)
        if (is_used(f))
            used_frames++;
}

/* ---------- Allocate / free ---------- */

void *pmm_alloc_block(void) {
    uint32_t flags = cpu_irq_save();
    void *result = 0;

    // Skip whole bytes that are full (8 used frames), then find the bit.
    for (uint32_t byte = 0; byte < (total_frames + 7) / 8 && !result; byte++) {
        if (bitmap[byte] == 0xFF)
            continue;
        for (uint32_t bit = 0; bit < 8; bit++) {
            uint32_t frame = byte * 8 + bit;
            if (frame < total_frames && !is_used(frame)) {
                mark_used(frame);
                used_frames++;
                result = (void *)(frame * PAGE_SIZE);
                break;
            }
        }
    }

    cpu_irq_restore(flags);
    return result;
}

void pmm_free_block(void *frame_address) {
    uint32_t flags = cpu_irq_save();

    // DEFENCE pmm_free_guard: aligned, real RAM, not reserved, in use.
    if (def_pmm_free_ok((uint32_t)frame_address)) {
        mark_free((uint32_t)frame_address / PAGE_SIZE);
        used_frames--;
    }

    cpu_irq_restore(flags);
}

/* ---------- Queries ---------- */

uint32_t pmm_get_max_blocks(void)  { return total_frames; }
uint32_t pmm_get_used_blocks(void) { return used_frames; }
uint32_t pmm_get_free_blocks(void) { return total_frames - used_frames; }

int pmm_is_used(uint32_t frame) {
    return frame >= total_frames || is_used(frame);
}

int pmm_is_reserved(uint32_t frame) {
    for (int i = 0; i < reserved_count; i++)
        if (frame >= reserved[i].first && frame < reserved[i].end)
            return 1;
    return 0;
}

int pmm_region_count(void) { return region_count; }

const pmm_region_t *pmm_region(int index) {
    return (index >= 0 && index < region_count) ? &regions[index] : 0;
}
