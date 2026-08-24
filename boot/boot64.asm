; boot64.asm
; This file contains the 64-bit entry point that is called immediately after 
; our 32-bit code switches the CPU into Long Mode.

section .text
bits 64
global long_mode_start
extern kernel_main

long_mode_start:
    ; 1. Load null into data segment registers. 
    ; In 64-bit mode, the CPU mostly ignores segment registers for data access,
    ; but loading them with 0 is considered good practice for a clean state.
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 2. Set up the stack pointer. 
    ; C code requires a valid stack to function properly.
    mov rsp, stack_top

    ; 3. Call the main C kernel function
    call kernel_main

    ; 4. In case kernel_main ever returns, enter an infinite safe halt loop
.halt:
    cli       ; Clear Interrupts (prevent interrupts from waking the CPU)
    hlt       ; Halt the CPU until the next interrupt (which will never come)
    jmp .halt ; Loop forever just in case an NMI (Non-Maskable Interrupt) wakes us

section .bss
align 16
stack_bottom:
    resb 16384 ; Reserve 16 KB for the kernel stack
stack_top:
