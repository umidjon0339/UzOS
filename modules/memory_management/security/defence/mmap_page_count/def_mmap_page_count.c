#include "mem_security.h"
#include "proc_security.h"
#include "vmm.h"

int mem_mmap_page_count(size_t length, size_t *pages) {
    if (!length || !pages) return 0;

    *pages = length / PAGE_SIZE;
    if (length % PAGE_SIZE) (*pages)++;
    return 1;
}

static int def_mmap_page_count_selftest(void) {
    size_t pages;
    size_t largest = (size_t)-1;
    size_t expected = largest / PAGE_SIZE;

    if (largest % PAGE_SIZE) expected++;
    if (!mem_mmap_page_count(1, &pages) || pages != 1) return 0;
    if (!mem_mmap_page_count(PAGE_SIZE, &pages) || pages != 1) return 0;
    if (!mem_mmap_page_count(PAGE_SIZE + 1, &pages) || pages != 2) return 0;
    if (!mem_mmap_page_count(largest, &pages) || pages != expected) return 0;
    if (mem_mmap_page_count(0, &pages)) return 0;
    if (mem_mmap_page_count(1, 0)) return 0;
    return 1;
}

static const defence_t def = {
    "mmap_page_count",
    "round mmap lengths without overflowing the requested page count",
    "mmap_length_wrap",
    def_mmap_page_count_selftest
};

void def_mmap_page_count_register(void) {
    proc_security_register_defence(&def);
}
