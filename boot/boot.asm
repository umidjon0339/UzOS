; boot.asm
; This file contains the Multiboot 1 header and the 32-bit entry point for our kernel.
; GRUB/QEMU loads the kernel in 32-bit protected mode. We must set up page tables,
; enable PAE, enable Long Mode, and load a 64-bit GDT before we can execute 64-bit C code.

; Multiboot 1 constants
MBOOT_MAGIC equ 0x1BADB002          ; Multiboot 1 magic number
MBOOT_FLAGS equ 0x00000007          ; Flags: page-align (0) + memory info (1) + video mode (2)
MBOOT_CHECKSUM equ -(MBOOT_MAGIC + MBOOT_FLAGS)

section .multiboot_header
align 4
header_start:
    dd MBOOT_MAGIC                   ; Magic number
    dd MBOOT_FLAGS                   ; Flags
    dd MBOOT_CHECKSUM                ; Checksum (magic + flags + checksum = 0)
    ; Address fields (unused with ELF, but required when flag bit 16 is not set)
    dd 0                             ; header_addr
    dd 0                             ; load_addr
    dd 0                             ; load_end_addr
    dd 0                             ; bss_end_addr
    dd 0                             ; entry_addr
    ; Video mode fields (required when flag bit 2 is set)
    dd 0                             ; mode_type: 0 = linear graphics
    dd 1024                          ; width
    dd 768                           ; height
    dd 32                            ; depth (bits per pixel)
header_end:

section .data
global multiboot_info_ptr
multiboot_info_ptr:
    dd 0                             ; Saved Multiboot info struct pointer (32-bit address)

section .bss
align 4096
pml4_table:
    resb 4096
pdp_table:
    resb 4096
pd_table:
    resb 4096 * 4 ; 4 PD tables to cover 4GB of physical address space


section .text
; Tell NASM to generate 32-bit instructions (since GRUB/QEMU drops us in 32-bit mode)
bits 32
global start
extern long_mode_start

start:
    ; Save the Multiboot info pointer (passed by GRUB in EBX) before we clobber registers
    mov [multiboot_info_ptr], ebx

    ; 1. Set up page tables to identity map the first 4GB of physical memory.
    ; This ensures the kernel can access framebuffer memory and any hardware in the lower 4GB.

    ; Point PML4[0] to PDP table
    mov eax, pdp_table
    or eax, 0b11 ; Set present and writable bits
    mov [pml4_table], eax

    ; Point PDP[0..3] to the 4 PD tables (covering 4GB)
    mov ecx, 0
.map_pdp:
    mov eax, pd_table
    mov edx, ecx
    shl edx, 12 ; Multiply index by 4096 bytes per PD table
    add eax, edx
    or eax, 0b11 ; Set present and writable bits
    mov [pdp_table + ecx * 8], eax
    mov dword [pdp_table + ecx * 8 + 4], 0
    inc ecx
    cmp ecx, 4
    jne .map_pdp

    ; Populate 2048 entries of 2MB each across the 4 PD tables (2048 * 2MB = 4GB)
    mov ecx, 0
    mov eax, 0x00000083 ; Present, Writable, Huge Page (bit 7)
    mov edx, 0
.map_pd:
    mov [pd_table + ecx * 8], eax
    mov [pd_table + ecx * 8 + 4], edx
    add eax, 0x200000
    adc edx, 0
    inc ecx
    cmp ecx, 2048
    jne .map_pd

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
