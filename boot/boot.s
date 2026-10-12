MBALIGN  equ  1 << 0            ; align loaded modules on page boundaries
MEMINFO  equ  1 << 1            ; provide memory map
FLAGS    equ  MBALIGN | MEMINFO ; this is the Multiboot 'flag' field
MAGIC    equ  0x1BADB002        ; 'magic number' lets bootloader find the header
CHECKSUM equ -(MAGIC + FLAGS)   ; checksum of above, to prove we are multiboot

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
resb 16384 ; 16 KiB
stack_top:

section .text
global _start:function (_start.end - _start)
extern kernel_main

_start:
    mov esp, stack_top
    ; Align stack and push arguments (right to left for cdecl):
    ; kernel_main(uint32_t multiboot_magic, multiboot_info_t *mb_info)
    push ebx ; Arg 2: Multiboot Information structure pointer
    push eax ; Arg 1: Multiboot Magic Number (0x2BADB002)
    call kernel_main


    cli
.hang:  hlt
    jmp .hang
.end:
