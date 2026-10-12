# Agent State Tracker

## Current Phase
Phase 4: Basic Memory Management and Paging.

## Implemented Modules
- Base structure created
- Multiboot header (boot.s)
- Basic kernel main (clears screen)
- Linker script and Makefile
- VGA Text Mode driver (clear screen, strings, integers, scrolling, cursor)
- GDT (Global Descriptor Table)
- IDT (Interrupt Descriptor Table) and CPU Exceptions
- Programmable Interrupt Controller (PIC) Remapping
- PIT (Programmable Interval Timer) Driver [IRQ0]
- PS/2 Keyboard Driver [IRQ1]
- Multiboot Memory Map Parsing (`include/multiboot.h`)
- Physical Memory Manager (PMM Bitmap Allocator in `modules/memory_management/pmm/`)
- Virtual Memory Manager (VMM 2-Level Paging CR3/CR0 in `modules/memory_management/vmm/`)
- Page Fault Handler (Vector 14)
- Kernel Heap Allocator (`kmalloc` / `kfree` in `modules/memory_management/kheap/`)

## Current Phase: Phase 5 - User Mode, System Calls, and Task Scheduling
- [ ] Task State Segment (TSS) setup
- [ ] User Mode Switch (`Ring 0` -> `Ring 3`)
- [ ] System Call Interface (`int 0x80`)
- [ ] Basic Preemptive Multitasking Scheduler (Round-Robin)

## Known Bugs/Issues
- None.


