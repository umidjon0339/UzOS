#include "mem_security.h"
#include "proc_security.h"
#include "vga.h"

static int atk_user_ptr_wrap_check(void) {
    static const struct {
        uint32_t start;
        size_t size;
        int expected;
    } probes[] = {
        { 0x1000u, 16, 1 },
        { KERNEL_SPACE_START - 1, 1, 1 },
        { KERNEL_SPACE_START - 1, 2, 0 },
        { 0xFFFFFFFFu, 2, 0 }
    };
    int refused = 0;

    for (unsigned int i = 0; i < sizeof(probes) / sizeof(probes[0]); i++) {
        int valid = mem_user_range_valid(probes[i].start, probes[i].size);
        if (valid != probes[i].expected) refused++;
    }
    return refused;
}

static int atk_user_ptr_wrap_simulate(void) {
    vga_print_str("  probes include a kernel-boundary crossing and uint32 wrap\n");
    vga_print_str("  no candidate address is dereferenced\n");
    return atk_user_ptr_wrap_check();
}

static int atk_user_ptr_wrap_launch(void) {
    vga_print_str("  probing the live user-range validator\n");
    return atk_user_ptr_wrap_check();
}

static const attack_t atk = {
    "user_ptr_wrap",
    "crafted pointer ranges can wrap or cross into kernel space",
    INV_TRUST,
    SEV_HIGH,
    atk_user_ptr_wrap_simulate,
    atk_user_ptr_wrap_launch,
    0,
    "user_ptr_bounds"
};

void atk_user_ptr_wrap_register(void) {
    proc_security_register_attack(&atk);
}
