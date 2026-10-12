global gdt_flush
global tss_flush

section .text
gdt_flush:
    ; The pointer to the GDT struct is passed as a parameter on the stack.
    ; ESP + 4 contains the first argument.
    mov eax, [esp + 4]  
    lgdt [eax]          ; Load the new GDT pointer

    ; Reload data segment registers with the Kernel Data Segment (offset 0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Reload code segment with a far jump to the Kernel Code Segment (offset 0x08)
    jmp 0x08:.flush
.flush:
    ret

tss_flush:
    mov ax, 0x2B      ; 0x2B is the TSS selector (5 * 8 = 40 = 0x28) | RPL 3 (0x3) = 0x2B
    ltr ax            ; Load Task Register
    ret
