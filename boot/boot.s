; =============================================================================
; boot.s - The first code that runs.
;
; The bootloader (GRUB or QEMU -kernel) finds the multiboot header below,
; loads the kernel at 1 MiB, and jumps to _start in 32-bit protected mode.
; Our only job here: set up a stack and call kernel_main() in C.
; =============================================================================

; --- Multiboot header (must be in the first 8 KiB of the file) --------------
MB_MAGIC    equ 0x1BADB002          ; bootloader searches for this number
MB_ALIGN    equ 1 << 0              ; load modules on page boundaries
MB_MEMINFO  equ 1 << 1              ; give us a memory map
MB_FLAGS    equ MB_ALIGN | MB_MEMINFO
MB_CHECKSUM equ -(MB_MAGIC + MB_FLAGS)  ; magic + flags + checksum must be 0

section .multiboot
align 4
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM

; --- Kernel stack (16 KiB) ---------------------------------------------------
; The stack grows down, so ESP starts at stack_top.
section .bss
align 16
global stack_bottom
global stack_top
stack_bottom:
    resb 16384
stack_top:

; --- Entry point -------------------------------------------------------------
; On entry: EAX = multiboot magic, EBX = pointer to multiboot info.
section .text
global _start:function (_start.end - _start)
extern kernel_main

_start:
    cli                     ; no interrupts until we have an IDT
    cld                     ; C code expects the direction flag clear

    mov esp, stack_top
    sub esp, 8              ; keep ESP 16-byte aligned at the call below

    push ebx                ; arg 2: multiboot_info_t *mb_info
    push eax                ; arg 1: uint32_t multiboot_magic
    call kernel_main

    ; kernel_main never returns. If it does, stop the CPU.
    cli
.hang:
    hlt
    jmp .hang
.end:
