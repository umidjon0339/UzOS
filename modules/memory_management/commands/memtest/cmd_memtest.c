/*
 * cmd_memtest.c - `memtest`: use sbrk, mmap and munmap and check them.
 *
 * For each syscall: get memory, write a pattern, read it back, give the
 * memory back. At the end every frame must be free again, otherwise
 * something leaked.
 *
 * Uses: sys_sbrk, sys_mmap, sys_munmap (syscalls/), pmm_get_free_blocks
 *       (core/pmm), ui_* (ui module).
 */

#include "mem_commands.h"
#include "sys_sbrk.h"
#include "sys_mmap.h"
#include "sys_munmap.h"
#include "pmm.h"
#include "ui.h"

#define SBRK_BYTES 10000
#define MMAP_BYTES (3 * PAGE_SIZE)

// Fill 'size' bytes with a pattern and read it back. 1 = memory works.
static int write_read_ok(uint8_t *mem, uint32_t size) {
    for (uint32_t i = 0; i < size; i++)
        mem[i] = (uint8_t)(i * 7);
    for (uint32_t i = 0; i < size; i++)
        if (mem[i] != (uint8_t)(i * 7))
            return 0;
    return 1;
}

static void report(const char *what, void *addr, int ok) {
    vga_print_str(what);
    vga_print_hex((uint32_t)addr);
    ui_color(ok ? UI_GOOD : UI_BAD);
    vga_print_str(ok ? "  " GLYPH_CHECK " write+read OK" : "  " GLYPH_CROSS " FAILED");
    ui_end();
}

void cmd_memtest(const char *args) {
    (void)args;
    uint32_t free_before = pmm_get_free_blocks();

    vga_print_str("free frames before : ");
    vga_print_int(free_before);
    vga_print_str("\n");

    // sbrk: grow the heap, use it, shrink it back.
    uint8_t *heap = sys_sbrk(SBRK_BYTES);
    report("sbrk(+10000)  -> ", heap, heap != SBRK_FAILED && write_read_ok(heap, SBRK_BYTES));
    if (heap != SBRK_FAILED)
        sys_sbrk(-SBRK_BYTES);

    // mmap: get a region, use it, unmap it.
    uint8_t *region = sys_mmap(MMAP_BYTES);
    report("mmap(12288)   -> ", region, region && write_read_ok(region, MMAP_BYTES));
    if (region)
        vga_print_str(sys_munmap(region, MMAP_BYTES) == 0 ? "munmap        -> 0 (unmapped)\n"
                                                          : "munmap        -> refused\n");

    uint32_t free_after = pmm_get_free_blocks();
    vga_print_str("free frames after  : ");
    vga_print_int(free_after);
    ui_color(free_after == free_before ? UI_GOOD : UI_BAD);
    vga_print_str(free_after == free_before ? "  " GLYPH_CHECK " every frame came back"
                                            : "  " GLYPH_CROSS " LEAK");
    ui_end();
}
