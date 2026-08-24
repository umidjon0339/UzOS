; isr.asm — IRQ stub for keyboard (IRQ1, vector 33)

section .text
bits 64

extern keyboard_handler  ; C function in keyboard.c

global irq1_handler
irq1_handler:
    ; Save all general-purpose registers
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11

    ; Call the C keyboard handler
    call keyboard_handler

    ; Send End-Of-Interrupt (EOI) to master PIC
    mov al, 0x20
    out 0x20, al

    ; Restore registers
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    iretq  ; Return from interrupt (64-bit)
