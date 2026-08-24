; boot.S
; This file contains the Multiboot1 header and the 32-bit entry point for our kernel.
; GRUB loads the kernel in 32-bit protected mode. We must set up page tables, 
; enable PAE, enable Long Mode, and load a 64-bit GDT before we can execute 64-bit C code.

section .multiboot_header
align 4
    dd 0x1BADB002            ; Multiboot 1 Magic number
    dd 0x03                  ; Flags (align modules, memory map)
    dd -(0x1BADB002 + 0x03)  ; Checksum

; Reserve space for the initial page tables in the .bss section
section .bss
align 4096
pml4_table:
    resb 4096
pdp_table:
    resb 4096
pd_table:
    resb 4096

section .text
; Tell NASM to generate 32-bit instructions (since GRUB drops us in 32-bit mode)
bits 32
global start
extern long_mode_start

start:
    ; 1. Set up page tables to identity map the first 2MB of memory.
    ; This allows us to access VGA text buffer (0xB8000) and our kernel at 1MB seamlessly.

    ; Point the first entry of the Page Map Level 4 (PML4) to the Page Directory Pointer (PDP) table
    mov eax, pdp_table
    or eax, 0b11 ; Set present and writable bits
    mov [pml4_table], eax

    ; Point the first entry of the PDP table to the Page Directory (PD) table
    mov eax, pd_table
    or eax, 0b11 ; Set present and writable bits
    mov [pdp_table], eax

    ; Point the first entry of the PD table to a 2MB huge page starting at physical address 0x0
    mov eax, 0x0
    or eax, 0b10000011 ; Set present, writable, and HUGE PAGE (bit 7) bits
    mov [pd_table], eax

    ; 2. Enable Physical Address Extension (PAE) in CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; 3. Move the address of the PML4 table to CR3 (Page-Directory Base Register)
    mov eax, pml4_table
    mov cr3, eax

    ; 4. Enable Long Mode (64-bit mode) by setting the LM bit in the EFER MSR
    mov ecx, 0xC0000080 ; EFER MSR number
    rdmsr
    or eax, 1 << 8      ; Set Long Mode Enable (LME) bit
    wrmsr

    ; 5. Enable Paging by setting the PG bit in CR0
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; 6. Load the 64-bit Global Descriptor Table (GDT)
    lgdt [gdt64.pointer]

    ; 7. Far jump to the 64-bit code segment to update the CS register and drop into 64-bit mode
    jmp gdt64.code_segment:long_mode_start

; The 64-bit GDT (Global Descriptor Table)
section .rodata
align 8
gdt64:
    dq 0 ; The zero entry (required)
.code_segment equ $ - gdt64
    ; Code segment descriptor: Executable (43), Descriptor Type (44), Present (47), 64-bit (53)
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
.pointer:
    dw $ - gdt64 - 1 ; Length of GDT minus 1
    dq gdt64         ; Pointer to GDT
