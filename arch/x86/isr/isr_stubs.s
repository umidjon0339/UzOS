; =============================================================================
; isr_stubs.s - Entry points for every interrupt.
;
; The CPU jumps here through the IDT. Each stub pushes the same frame
; (error code + vector number), then all share isr_common, which saves
; the registers and calls isr_handler(registers_t *) in C.
; =============================================================================

KERNEL_DATA equ 0x10

; --- One stub per vector -----------------------------------------------------
; Some exceptions push an error code, the rest do not. We push a fake 0 for
; those so the stack always has the same layout.
; Use 'dword': 'push byte 128' would sign-extend to 0xFFFFFF80.

%macro ISR_NO_ERR 1             ; CPU pushes no error code
global isr%1
isr%1:
    push dword 0                ; fake error code
    push dword %1               ; vector number
    jmp isr_common
%endmacro

%macro ISR_ERR 1                ; CPU already pushed an error code
global isr%1
isr%1:
    push dword %1               ; vector number
    jmp isr_common
%endmacro

%macro IRQ 2                    ; %1 = IRQ line, %2 = vector
global irq%1
irq%1:
    push dword 0                ; IRQs have no error code
    push dword %2
    jmp isr_common
%endmacro

; CPU exceptions 0-31 (8, 10-14, 17, 21, 29, 30 push an error code)
ISR_NO_ERR 0
ISR_NO_ERR 1
ISR_NO_ERR 2
ISR_NO_ERR 3
ISR_NO_ERR 4
ISR_NO_ERR 5
ISR_NO_ERR 6
ISR_NO_ERR 7
ISR_ERR    8
ISR_NO_ERR 9
ISR_ERR    10
ISR_ERR    11
ISR_ERR    12
ISR_ERR    13
ISR_ERR    14
ISR_NO_ERR 15
ISR_NO_ERR 16
ISR_ERR    17
ISR_NO_ERR 18
ISR_NO_ERR 19
ISR_NO_ERR 20
ISR_ERR    21
ISR_NO_ERR 22
ISR_NO_ERR 23
ISR_NO_ERR 24
ISR_NO_ERR 25
ISR_NO_ERR 26
ISR_NO_ERR 27
ISR_NO_ERR 28
ISR_ERR    29
ISR_ERR    30
ISR_NO_ERR 31

; Hardware IRQs 0-15 -> vectors 32-47
IRQ 0,  32
IRQ 1,  33
IRQ 2,  34
IRQ 3,  35
IRQ 4,  36
IRQ 5,  37
IRQ 6,  38
IRQ 7,  39
IRQ 8,  40
IRQ 9,  41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; System call
ISR_NO_ERR 128

; --- Shared part -------------------------------------------------------------
extern isr_handler

isr_common:
    pusha                       ; save eax, ecx, edx, ebx, esp, ebp, esi, edi

    mov ax, ds                  ; save the caller's data segment
    push eax

    mov ax, KERNEL_DATA         ; use kernel segments while in C
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    cld                         ; C expects DF=0; user code may have set it

    push esp                    ; arg: registers_t *regs (top of stack)
    call isr_handler
    add esp, 4

    pop eax                     ; restore the caller's data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8                  ; drop vector number + error code
    iret                        ; restore EIP, CS, EFLAGS (and ESP, SS)

; --- Address tables for idt.c ------------------------------------------------
section .rodata

global isr_stub_table
isr_stub_table:
%assign i 0
%rep 32
    dd isr %+ i
%assign i i + 1
%endrep

global irq_stub_table
irq_stub_table:
%assign i 0
%rep 16
    dd irq %+ i
%assign i i + 1
%endrep
