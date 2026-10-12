#include "vmm.h"
#include "pmm.h"
#include "vga.h"
#include "isr.h"

static page_directory_t *kernel_directory = NULL;
static page_directory_t *current_directory = NULL;
static page_table_t *kernel_tables_virt[TABLES_PER_DIR];

static inline void load_page_directory(page_directory_t *dir) {
    asm volatile ("mov %0, %%cr3" : : "r"(dir) : "memory");
}

static inline void enable_paging(void) {
    uint32_t cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    asm volatile ("mov %0, %%cr0" : : "r"(cr0) : "memory");
}


void page_fault_handler(registers_t *regs) {
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    int present  = !(regs->err_code & 0x1);
    int write    = regs->err_code & 0x2;
    int user     = regs->err_code & 0x4;
    int reserved = regs->err_code & 0x8;
    int id       = regs->err_code & 0x10;

    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_print_str("\n================ PAGE FAULT ================\n");
    vga_print_str("Faulting Address: ");
    vga_print_hex(faulting_address);
    vga_print_str("\nReason: ");
    if (present)  vga_print_str("[Page Not Present] ");
    if (write)    vga_print_str("[Write Violation] ");
    if (user)     vga_print_str("[User Mode] ");
    if (reserved) vga_print_str("[Reserved Bit] ");
    if (id)       vga_print_str("[Instruction Fetch] ");
    vga_print_str("\nSystem Halted!\n");

    while (1) {
        asm volatile("hlt");
    }
}

void vmm_map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x03FF;

    pde_t *pde = &current_directory->tables[pd_index];

    if (!(*pde & PAGE_PRESENT)) {
        void *pt_phys = pmm_alloc_block();
        if (!pt_phys) return;

        uint32_t *pt_ptr = (uint32_t *)pt_phys;
        for (int i = 0; i < PAGES_PER_TABLE; i++) {
            pt_ptr[i] = 0;
        }

        *pde = (uint32_t)pt_phys | PAGE_PRESENT | PAGE_WRITABLE | flags;
        kernel_tables_virt[pd_index] = (page_table_t *)pt_phys;
    }

    page_table_t *table = kernel_tables_virt[pd_index];
    table->pages[pt_index] = (phys_addr & 0xFFFFF000) | PAGE_PRESENT | flags;
}

void vmm_unmap_page(uint32_t virt_addr) {
    uint32_t pd_index = virt_addr >> 22;
    uint32_t pt_index = (virt_addr >> 12) & 0x03FF;

    pde_t *pde = &current_directory->tables[pd_index];
    if (!(*pde & PAGE_PRESENT)) return;

    page_table_t *table = kernel_tables_virt[pd_index];
    table->pages[pt_index] = 0;

    asm volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
}

void vmm_init(void) {
    register_interrupt_handler(14, page_fault_handler);

    kernel_directory = (page_directory_t *)pmm_alloc_block();
    if (!kernel_directory) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_print_str("[VMM ERROR] Out of physical RAM for Kernel Page Directory!\n");
        return;
    }

    for (int i = 0; i < TABLES_PER_DIR; i++) {
        kernel_directory->tables[i] = 0;
        kernel_tables_virt[i] = NULL;
    }
    current_directory = kernel_directory;

    // Identity Map first 8 MB of memory (0x00000000 - 0x00800000)
    for (uint32_t i = 0; i < 0x00800000; i += PAGE_SIZE) {
        vmm_map_page(i, i, PAGE_WRITABLE);
    }
    
    // Identity Map 8 MB for QEMU BGA Framebuffer (0xFD000000 - 0xFD800000)
    for (uint32_t i = 0xFD000000; i < 0xFD800000; i += PAGE_SIZE) {
        vmm_map_page(i, i, PAGE_WRITABLE);
    }

    load_page_directory(kernel_directory);
    enable_paging();
}


