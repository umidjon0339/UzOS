#include <stdint.h>
#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "timer.h"
#include "keyboard.h"
#include "multiboot.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "shell.h"
#include "shell_commands.h"
#include "boot_logo.h"
#include "mem_commands.h"
#include "timer_commands.h"
#include "process.h"
#include "scheduler.h"
#include "proc_commands.h"
#include "ramdisk.h"
#include "fs_commands.h"
#include "gui_commands.h"
#include "proc_security.h"

extern void init_syscalls(void);

void kernel_main(uint32_t multiboot_magic, multiboot_info_t *mb_info) {
    // Initialize VGA Driver
    vga_init();
    
    // Display the boot logo
    print_boot_logo();
    
    // 1. Initialize Global Descriptor Table
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Initializing GDT... ");
    init_gdt();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("[SUCCESS]\n");

    // 2. Initialize Interrupt Descriptor Table
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Initializing IDT... ");
    init_idt();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("[SUCCESS]\n");

    // 3. Remap Programmable Interrupt Controller
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Remapping PIC (IRQs 0-15 to Vectors 32-47)... ");
    pic_remap(0x20, 0x28);
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("[SUCCESS]\n");

    // 4. Initialize Hardware Drivers (PIT & Keyboard)
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Initializing PIT Timer... ");
    init_timer(100); // 100 Hz
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("[SUCCESS]\n");

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Initializing PS/2 Keyboard Driver... ");
    init_keyboard();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("[SUCCESS]\n");

    // 5. Initialize Memory Management Subsystem
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Initializing Physical Memory Manager (PMM)... ");
    if (multiboot_magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("[ERROR] Invalid Multiboot Magic Number!\n");
    } else {
        pmm_init(mb_info);
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_print_str("[SUCCESS]\n");

        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        vga_print_str(" -> Total RAM: ");
        vga_print_int((pmm_get_max_blocks() * 4) / 1024);
        vga_print_str(" MB | Free Page Frames: ");
        vga_print_int(pmm_get_free_blocks());
        vga_print_str("\n");
    }

    // 6. Initialize Virtual Memory Manager & Activate Paging
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Initializing Virtual Memory Manager & Paging (CR0)... ");
    vmm_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("[SUCCESS]\n");

    // 7. Initialize Kernel Heap Allocator
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Initializing Kernel Heap Allocator (kmalloc)... ");
    kheap_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("[SUCCESS]\n");

    // Register security demonstrations across the kernel subsystems.
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Initializing System Security Lab... ");
    proc_security_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("[SUCCESS]\n");

    // 8. Initialize Shell & Register Commands from Each Module
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Initializing Shell... ");
    shell_init();
    register_shell_commands();   // shell/commands/ -> help, clear, echo
    register_mem_commands();     // memory_management/commands/ -> meminfo, heapinfo
    register_timer_commands();   // device_management/timer/commands/ -> uptime
    register_proc_commands();    // process_management/commands/ -> ps, kill, spawn
    register_fs_commands();      // filesystem_management/commands/ -> ls, mkdir, touch, cat, write, rm
    register_gui_commands();     // device_management/gui/commands/ -> startx
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("[SUCCESS]\n");
    
    // Initialize Virtual Filesystem
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Initializing Virtual Filesystem & RAMDisk... ");
    ramdisk_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("[SUCCESS]\n");

    // Initialize Process Management (Multitasking)
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Initializing Process Management... ");
    scheduler_init();
    
    // Create an "idle" or "main" process representing the current execution thread.
    // We don't need a real entry point for it since it's already running.
    // But we need a slot so the scheduler has something to switch FROM.
    int main_pid = process_create("kernel_main", NULL, 0);
    process_t *main_proc = process_get_by_pid(main_pid);
    if (main_proc) {
        main_proc->state = PROCESS_STATE_RUNNING;
        // Since kernel_main stack isn't the allocated one, we free it to save 4KB!
        // Actually, kheap kfree might complain if we aren't careful, but it's safe.
        // For simplicity, let's just leave it or we can just not allocate stack if entry_point == NULL
    }
    
    scheduler_set_current_process(main_pid);
    
    scheduler_start();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("[SUCCESS]\n");

    // 9. Initialize System Calls
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    vga_print_str("Initializing Syscalls (int 0x80)... ");
    init_syscalls();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_print_str("[SUCCESS]\n");

    // 10. Enable Hardware Interrupts!
    vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    vga_print_str("Enabling Hardware Interrupts (sti)...\n");
    asm volatile("sti");

    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_print_str("\nuzos is ready! Type 'help' for available commands.\n/> ");

    // Idle loop waiting for interrupts and processing shell
    while (1) {
        char c = keyboard_pop_key();
        if (c) {
            shell_handle_key(c);
        } else {
            asm volatile("hlt");
        }
    }
}
