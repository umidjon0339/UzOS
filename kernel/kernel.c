#include "terminal.h"
#include "idt.h"
#include "shell.h"
#include <stdint.h>

/* Multiboot 1 info structure (only the fields we need).
 * The full struct is defined in the Multiboot 1 spec.
 * Offsets and field order follow the spec exactly. */
struct multiboot_info {
    uint32_t flags;            /* 0 */
    uint32_t mem_lower;        /* 4 */
    uint32_t mem_upper;        /* 8 */
    uint32_t boot_device;      /* 12 */
    uint32_t cmdline;          /* 16 */
    uint32_t mods_count;       /* 20 */
    uint32_t mods_addr;        /* 24 */
    uint32_t syms[4];          /* 28-44 */
    uint32_t mmap_length;      /* 44 */
    uint32_t mmap_addr;        /* 48 */
    uint32_t drives_length;    /* 52 */
    uint32_t drives_addr;      /* 56 */
    uint32_t config_table;     /* 60 */
    uint32_t boot_loader_name; /* 64 */
    uint32_t apm_table;        /* 68 */
    /* VBE info */
    uint32_t vbe_control_info;  /* 72 */
    uint32_t vbe_mode_info;     /* 76 */
    uint16_t vbe_mode;          /* 80 */
    uint16_t vbe_interface_seg; /* 82 */
    uint16_t vbe_interface_off; /* 84 */
    uint16_t vbe_interface_len; /* 86 */
    /* Framebuffer info (Multiboot 1 extension, flag bit 12) */
    uint64_t framebuffer_addr;  /* 88 */
    uint32_t framebuffer_pitch; /* 96 */
    uint32_t framebuffer_width; /* 100 */
    uint32_t framebuffer_height;/* 104 */
    uint8_t  framebuffer_bpp;   /* 108 */
    uint8_t  framebuffer_type;  /* 109 */
} __attribute__((packed));

void kernel_main(uint32_t mbi_addr) {
    struct multiboot_info* mbi = (struct multiboot_info*)(uint64_t)mbi_addr;

    /* Extract framebuffer info from multiboot.
     * Bit 12 of flags means framebuffer info is available. */
    uint32_t* fb = (uint32_t*)(uint64_t)mbi->framebuffer_addr;
    uint32_t  fb_width  = mbi->framebuffer_width;
    uint32_t  fb_height = mbi->framebuffer_height;
    uint32_t  fb_pitch  = mbi->framebuffer_pitch;
    uint8_t   fb_bpp    = mbi->framebuffer_bpp;

    /* Initialize framebuffer terminal */
    terminal_initialize(fb, fb_width, fb_height, fb_pitch, fb_bpp);

    /* Print welcome banner */
    terminal_print("================================\n");
    terminal_print("          Welcome to MyOS\n");
    terminal_print("================================\n\n");
    terminal_print("MyOS v0.1 | x86_64\n\n");

    /* Set up interrupts (IDT + PIC + keyboard IRQ) */
    idt_init();

    /* Start the interactive shell (never returns) */
    shell_run();
}
