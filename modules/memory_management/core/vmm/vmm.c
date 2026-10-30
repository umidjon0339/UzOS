/*
 * vmm.c - Build the page tables, map pages, handle page faults.
 *
 * How the kernel reaches its own page tables
 * ------------------------------------------
 * A new page table lives in a free frame from the PMM, which can be
 * anywhere in RAM. Writing to it through its physical address only works
 * while that address happens to be identity mapped (the first 8 MiB).
 *
 * The fix is the "recursive" trick: the LAST directory entry points at
 * the directory itself. The CPU then treats the directory as a page
 * table, so page table i shows up at virtual address
 *
 *     PAGE_TABLES_VIRT + i * PAGE_SIZE          (0xFFC00000 + i * 4096)
 *
 * wherever its frame really is.
 *
 * Uses: pmm_alloc_block (core/pmm), register_interrupt_handler
 *       (arch/x86/isr), process_exit (process_management) to stop a
 *       user program that faults, the defence null_page, cpu_fault_record
 *       and the defence fault_isolation (cpu_management) to decide who
 *       pays for a fault, and ui_* (ui module) for the fault report.
 */

#include "vmm.h"
#include "pmm.h"
#include "isr.h"
#include "cpu.h"
#include "ui.h"
#include "scheduler.h"
#include "signal.h"          // SIGSEGV
#include "def_null_page.h"
#include "cpu_faults.h"
#include "def_fault_isolation.h"

#define RECURSIVE_SLOT   1023               // directory entry that points at itself
#define PAGE_FAULT       14                 // CPU exception number

static page_directory_t *directory;         // in the first 8 MiB: phys == virt
static int paging_on;

static uint32_t dir_index(uint32_t virt)   { return virt >> 22; }
static uint32_t table_index(uint32_t virt) { return (virt >> 12) & 0x3FF; }

// Where the kernel can read/write page table 'i' right now.
static page_table_t *table_at(uint32_t i) {
    if (paging_on)
        return (page_table_t *)(PAGE_TABLES_VIRT + i * PAGE_SIZE);
    return (page_table_t *)(directory->tables[i] & ~PAGE_FLAGS);   // paging off: physical works
}

static void flush_tlb(uint32_t virt) {
    if (paging_on)
        asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

/* ---------- Mapping ---------- */

int vmm_map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t irq = cpu_irq_save();
    pde_t *pde = &directory->tables[dir_index(virt)];

    if (!(*pde & PAGE_PRESENT)) {
        // No page table covers this 4 MiB yet: make one from a free frame.
        void *frame = pmm_alloc_block();
        if (!frame) {
            cpu_irq_restore(irq);
            return 0;
        }
        *pde = (uint32_t)frame | PAGE_PRESENT | PAGE_WRITABLE | (flags & PAGE_USER);
        flush_tlb((uint32_t)table_at(dir_index(virt)));

        page_table_t *table = table_at(dir_index(virt));
        for (int i = 0; i < PAGES_PER_TABLE; i++)
            table->pages[i] = 0;
    } else if (flags & PAGE_USER) {
        // The CPU checks BOTH levels: a user page needs a user table too.
        *pde |= PAGE_USER;
    }

    table_at(dir_index(virt))->pages[table_index(virt)] =
        (phys & ~PAGE_FLAGS) | PAGE_PRESENT | flags;
    flush_tlb(virt);    // forget any old translation of this page

    cpu_irq_restore(irq);
    return 1;
}

static int table_is_empty(page_table_t *table) {
    for (int i = 0; i < PAGES_PER_TABLE; i++)
        if (table->pages[i])
            return 0;
    return 1;
}

void vmm_unmap_page(uint32_t virt) {
    uint32_t irq = cpu_irq_save();
    pde_t *pde = &directory->tables[dir_index(virt)];

    if (*pde & PAGE_PRESENT) {
        page_table_t *table = table_at(dir_index(virt));
        table->pages[table_index(virt)] = 0;
        flush_tlb(virt);

        // Last page of this 4 MiB gone: give the page table's frame back too.
        if (table_is_empty(table)) {
            uint32_t frame = *pde & ~PAGE_FLAGS;
            *pde = 0;
            flush_tlb((uint32_t)table);
            pmm_free_block((void *)frame);
        }
    }

    cpu_irq_restore(irq);
}

int vmm_walk(uint32_t virt, vmm_walk_t *out) {
    out->dir_index   = dir_index(virt);
    out->table_index = table_index(virt);
    out->pde         = directory->tables[out->dir_index];
    out->pte         = 0;
    out->physical    = 0;

    if (!(out->pde & PAGE_PRESENT))
        return 0;

    out->pte = table_at(out->dir_index)->pages[out->table_index];
    if (!(out->pte & PAGE_PRESENT))
        return 0;

    out->physical = (out->pte & ~PAGE_FLAGS) | (virt & PAGE_FLAGS);
    return 1;
}

/* ---------- Page faults ---------- */

static void print_fault(uint32_t addr, registers_t *regs) {
    uint32_t err = regs->err_code;

    vga_print_str("Address: ");
    vga_print_hex(addr);
    vga_print_str("  EIP: ");
    vga_print_hex(regs->eip);
    vga_print_str("\nReason : ");
    vga_print_str(err & 0x1 ? "[protection violation] " : "[page not present] ");
    vga_print_str(err & 0x2 ? "[write] " : "[read] ");
    if (err & 0x10) vga_print_str("[instruction fetch] ");
    if (addr < NULL_PAGE_END) vga_print_str("[NULL pointer]");
    vga_print_str("\n");
}

static void page_fault_handler(registers_t *regs) {
    uint32_t addr;
    asm volatile("mov %%cr2, %0" : "=r"(addr));    // CR2 = the address that faulted

    // DEFENCE fault_isolation (cpu_management): a user program's bug
    // must not take the whole OS down: stop just it.
    if (def_fault_isolation_survivable(regs)) {
        cpu_fault_record(regs);         // "[CPU FAULT] Page Fault (#14) in PID ..."
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        print_fault(addr, regs);
        ui_reset();
        process_exit_with(SIGSEGV | 0x80);  // as wait() reports it: SIGSEGV, core dumped
    }

    // A fault in kernel code is a kernel bug: stop everything.
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_fullscreen(0);              // a full-screen program must not hide this
    vga_print_str("\n================ PAGE FAULT (kernel) ================\n");
    print_fault(addr, regs);
    vga_print_str("System Halted!\n");
    asm volatile("cli");
    while (1)
        asm volatile("hlt");
}

/* ---------- Setup ---------- */

static void identity_map(uint32_t start, uint32_t end) {
    for (uint32_t addr = start; addr < end; addr += PAGE_SIZE)
        vmm_map_page(addr, addr, PAGE_WRITABLE);
}

void vmm_init(void) {
    register_interrupt_handler(PAGE_FAULT, page_fault_handler);

    directory = pmm_alloc_block();     // early frame: inside the first 8 MiB
    if (!directory) {
        vga_print_str("[VMM ERROR] No RAM for the page directory!\n");
        return;
    }
    for (int i = 0; i < TABLES_PER_DIR; i++)
        directory->tables[i] = 0;

    directory->tables[RECURSIVE_SLOT] = (uint32_t)directory | PAGE_PRESENT | PAGE_WRITABLE;

    identity_map(0, IDENTITY_MAP_END);                   // kernel, VGA, boot data
    identity_map(FRAMEBUFFER_START, FRAMEBUFFER_END);    // GUI video memory

    // DEFENCE null_page: leave page 0 unmapped so NULL accesses fault.
    def_null_page_protect();

    asm volatile("mov %0, %%cr3" : : "r"(directory) : "memory");   // load the directory

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    asm volatile("mov %0, %%cr0" : : "r"(cr0 | 0x80000000) : "memory");  // PG bit: paging on
    paging_on = 1;
}
