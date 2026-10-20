/*
 * sys_sbrk.c - sbrk(): move the "program break", the end of the heap.
 *
 *   USER_BRK_START            break                     USER_BRK_END
 *        |=========== heap ===========|      free...       |
 *
 * sbrk(+n) moves the break up by n bytes and maps any new pages it now
 * covers; sbrk(-n) moves it down and unmaps whole pages it left. It
 * returns the OLD break, so sbrk(n) == "here are your n new bytes".
 * malloc() in a C library is built on top of exactly this.
 *
 * Uses: umem_map / umem_unmap (core/umem), cpu_irq_save (arch/x86/cpu).
 * For now there is one break for the whole system, because all processes
 * share one address space.
 */

#include "sys_sbrk.h"
#include "umem.h"
#include "memory_layout.h"
#include "cpu.h"

static uint32_t brk = USER_BRK_START;

static uint32_t page_up(uint32_t addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

void *sys_sbrk(int32_t increment) {
    uint32_t irq = cpu_irq_save();
    void *result = SBRK_FAILED;

    // 64-bit math, so a huge increment cannot wrap around 4 GiB.
    int64_t wanted = (int64_t)brk + increment;

    if (wanted >= USER_BRK_START && wanted <= USER_BRK_END) {
        uint32_t new_brk = (uint32_t)wanted;
        uint32_t old_top = page_up(brk);        // first page not mapped yet
        uint32_t new_top = page_up(new_brk);
        int ok = 1;

        if (new_top > old_top)
            ok = umem_map(old_top, (new_top - old_top) / PAGE_SIZE);
        else if (new_top < old_top)
            umem_unmap(new_top, (old_top - new_top) / PAGE_SIZE);

        if (ok) {
            result = (void *)brk;
            brk = new_brk;
        }
    }

    cpu_irq_restore(irq);
    return result;
}
