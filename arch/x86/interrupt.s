global idt_flush

section .text
idt_flush:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; ISR Macros
%macro ISR_NOERRCODE 1
  global isr%1
  isr%1:
    cli
    push byte 0    ; Dummy error code
    push byte %1   ; Interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
  global isr%1
  isr%1:
    cli
    ; Error code is already pushed by CPU
    push byte %1
    jmp isr_common_stub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31
ISR_NOERRCODE 128 ; System call interrupt (0x80)

extern isr_handler

isr_common_stub:
    pusha           ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

    mov ax, ds      ; Lower 16-bits of eax = ds.
    push eax        ; save the data segment descriptor

    mov ax, 0x10    ; load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp        ; Pass pointer to registers_t as argument to isr_handler
    call isr_handler
    add esp, 4      ; Clean up the pushed esp

    pop eax         ; reload the original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa            ; Pops edi,esi,ebp,esp,ebx,edx,ecx,eax
    add esp, 8      ; Cleans up the pushed error code and pushed ISR number
    sti
    iret

; IRQ Macros
%macro IRQ 2
  global irq%1
  irq%1:
    cli
    push byte 0    ; Dummy error code
    push byte %2   ; Interrupt number (mapped IDT vector)
    jmp irq_common_stub
%endmacro

IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

extern irq_handler

irq_common_stub:
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp
    call irq_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8
    sti
    iret

global switch_to_user_mode
switch_to_user_mode:
    cli
    
    ; The arguments passed to this function:
    ; [esp + 4] = Entry point (EIP)
    ; [esp + 8] = User stack pointer (ESP)
    mov ebx, [esp + 4]
    mov ecx, [esp + 8]
    
    ; Set up Data Segment registers for Ring 3
    ; 0x23 = 0x20 (User Data Segment index 4) | 0x3 (RPL 3)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Setup the stack for IRET
    push 0x23          ; SS
    push ecx           ; ESP
    
    pushf              ; Push current EFLAGS
    pop eax
    or eax, 0x200      ; Set IF (Interrupt Enable Flag)
    push eax           ; EFLAGS
    
    push 0x1B          ; CS: 0x18 (User Code Segment index 3) | 0x3 (RPL 3)
    push ebx           ; EIP
    
    iret               ; Jump to user mode!
