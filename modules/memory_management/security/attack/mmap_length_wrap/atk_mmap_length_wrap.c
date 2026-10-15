#include "mem_security.h"
#include "proc_security.h"
#include "vga.h"
#include "vmm.h"

static int atk_mmap_length_wrap_check(void) {
    size_t length = (size_t)-1;
    size_t pages;
    size_t expected = length / PAGE_SIZE;

    if (length % PAGE_SIZE) expected++;
    if (!mem_mmap_page_count(length, &pages)) return 0;
    return pages == expected;
}

static int atk_mmap_length_wrap_simulate(void) {
    vga_print_str("  largest size_t request must round up without addition overflow\n");
    vga_print_str("  expected pages: ");
    vga_print_int((int)((size_t)-1 / PAGE_SIZE + (((size_t)-1 % PAGE_SIZE) != 0)));
    vga_print_str("\n");
    return atk_mmap_length_wrap_check();
}

static int atk_mmap_length_wrap_launch(void) {
    vga_print_str("  checking the live page-count policy; no pages are allocated\n");
    return atk_mmap_length_wrap_check();
}

static const attack_t atk = {
    "mmap_length_wrap",
    "page rounding can overflow and truncate a requested mapping length",
    INV_RESOURCE,
    SEV_MEDIUM,
    atk_mmap_length_wrap_simulate,
    atk_mmap_length_wrap_launch,
    0,
    "mmap_page_count"
};

void atk_mmap_length_wrap_register(void) {
    proc_security_register_attack(&atk);
}
